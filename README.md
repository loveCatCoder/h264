## H264File
- 注意负责读取文件内容
- 在文件buff中寻找Nalu
- 记录当前位置和文件总大小

## H264Buffer和H264Nalu合并
- 存储解析nalu header和nalu body的数据
- 根据nalu类型的不同实现不同的解析方法