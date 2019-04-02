# Problem

- 编译项目的时候, 遇到 无法解析符号 WSA 开头的符号

  Encounterd unresolved reference WSA* when compiling project
  
- 使用自己的类, 遇到无法解析的外部符号 LNK2019

  Encounterd unresolved external symbol, LNK2019 error
  
# Solution

- 项目属性->链接器->输入: 附加依赖项 添加 `ws2_32.lib;`

  Project Property->Linker->Input: add `ws2_32.lib;` to  Additional Dependencies
  
- 项目属性->链接器->输入: 忽略所有默认库 `否`

  Project Property->Linker->Input: set option "Ignore All Default Libraries" as `No`
