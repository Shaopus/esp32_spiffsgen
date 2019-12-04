[![Board Status](https://dev.azure.com/shaopus/07ef16c3-e1ad-44cb-8661-31783bea604b/9bb5a54c-afbc-4f81-8cad-ca2714e54ace/_apis/work/boardbadge/841fffac-dcd6-40fd-87ca-83cebec3ec14)](https://dev.azure.com/shaopus/07ef16c3-e1ad-44cb-8661-31783bea604b/_boards/board/t/9bb5a54c-afbc-4f81-8cad-ca2714e54ace/Microsoft.RequirementCategory)
# SPIFFS文件系统的使用步骤

## 概述

SPIFFS 是一个用于 SPI NOR flash 设备的嵌入式文件系统，支持磨损均衡、文件系统一致性检查等功能。 

## 说明

- 目前，SPIFFS 尚不支持目录，但可以生成**扁平结构**。如果 SPIFFS 挂载在 `/spiffs` 下，在 `/spiffs/tmp/myfile.txt` 路径下创建一个文件则会在 SPIFFS 中生成一个名为 `/tmp/myfile.txt` 的文件，而不是在 `/spiffs/tmp` 下生成名为 `myfile.txt` 的文件；
- SPIFFS 并非实时栈，每次写操作耗时不等；
- 目前，SPIFFS 尚不支持检测或处理已损坏的块。

## 工具

### mkspiffs

使用 [mkspiffs](https://github.com/igrr/mkspiffs) 工具创建 SPIFFS 分区映像。[mkspiffs](https://github.com/igrr/mkspiffs) 也可以用于从指定文件夹中生成映像，然后使用 `esptool.py` 烧录映像。 在项目目录下，下载[mkspiffs](https://github.com/igrr/mkspiffs)。

```shell
git clone https://github.com/igrr/mkspiffs
cd mkspiffs
git submodule update --init --recursive
```

该工具需要获取以下参数：

- **Block Size**：4096（SPI flash 标准）
- **Page Size**：256（SPI flash 标准）
- **Image Size**：分区大小（以字节为单位，可从分区表中获取）
- **Partition Offset**：分区起始地址（可从分区表内获取）

 运行以下命令，将文件夹打包成 1 MB 大小的映像: 

```shell
mkspiffs -c [src_folder] -b 4096 -p 256 -s 0x100000 spiffs.bin
```

 运行以下命令，将映像烧录到 ESP32（偏移量：0x110000）: 

```shell
python esptool.py --chip esp32 --port [port] --baud [baud] write_flash -z 0x110000 spiffs.bin
```

###  [分区表](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/api-guides/partition-tables.html) 

#### 创建自定义分区表

 如果在 `menuconfig` 中选择了 “Custom partition table CSV”，则还需要输入该分区表的 CSV 文件在项目中的路径。CSV 文件可以根据需要，描述任意数量的分区信息，工程的CSV文件如下：

```
# Name,   Type, SubType, Offset,   Size
nvs,      data, nvs,     0x009000, 0x004000
otadata,  data, ota,     0x00d000, 0x002000
phy_init, data, phy,     0x00f000, 0x001000
ota_0,    app,  ota_0,   0x010000, 0x180000
ota_1,    app,  ota_1,   0x190000, 0x180000
fs_0,     data, spiffs,  0x310000, 0x040000
fs_1,     data, spiffs,  0x350000, 0x040000
```

**特别注意**：

在 `menuconfig` 中选择了  `Component config `中的 `SPIFFS Configuration` ，将 `Size of per-file metadata field` 大小设置成`0`。**我一开始使用默认值，测试的时候怎么都读不出来。**

### makefile

因为每次修改文件，然后生成spiffs要打包，还要烧录，现结合上面命令，将一下命令结合在工程的makefile中。

```makefile
MKSPIFFS=$(PROJECT_PATH)/mkspiffs/mkspiffs
SPIFFS_IMAGE=$(BUILD_DIR_BASE)/spiffs.bin

# Note: The $(shell ...) hack is to start a clean make session
# Clean mkspiffs
clean: clean_spiffs

clean_spiffs:
	echo $(shell make -C $(PROJECT_PATH)/mkspiffs clean)

# Build mkspiffs utility
$(MKSPIFFS):
	echo $(shell make -C $(PROJECT_PATH)/mkspiffs)

SPIFFS_PARTITION=$(shell grep "^fs_0" partitions.csv | sed 's/,//g')
SPIFFS_OFFSET=$(word 4, $(SPIFFS_PARTITION))
SPIFFS_SIZE=$(word 5, $(SPIFFS_PARTITION))

# Build SPIFFS image
$(SPIFFS_IMAGE): $(PROJECT_PATH)/data $(MKSPIFFS) partitions.csv 
	$(MKSPIFFS) -c $< -b 4096 -p 256 -s $(SPIFFS_SIZE) $@

# Need to generate SPIFFS image before flashing
flash: $(SPIFFS_IMAGE)

# Include SPIFFS offset + image in the flash command
ESPTOOL_ALL_FLASH_ARGS += $(SPIFFS_OFFSET) $(SPIFFS_IMAGE) \
  $$(($(SPIFFS_OFFSET) + $(SPIFFS_SIZE))) $(SPIFFS_IMAGE)
```

## 总结

将上述操作总结如下：

获取[mkspiffs](https://github.com/igrr/mkspiffs)  -> 修改分区表 -> 修改makefile -> 编译