# ffmpeg 格式转换 视频剪辑 视频合并 播放速度

## 格式转换

`$ffmpeg -i nobody.flv nobody.mp4`

## 视频剪辑

`$ffmpeg -ss 00:00:00 -t 00:00:30 -i nobody.mp4 -vcodec copy -acodec copy nobody_00_30.mp4`
* -ss 指定从什么时间开始 (00:00:30)
* -t  指定需要截取的时长 (30秒)
* -i  输入文件

这样剪辑会在起始和结束时间前后自动寻找关键帧, 会有几秒误差

### 视频剪辑-精准版

- 先将视频转化为帧内编码
  
  `$ffmpeg -i nobody.mp4 -qscale 0 -intra nobody_entire.mp4`
  然后在重复**视频剪辑**, 将输入文件 `nobody.mp4` 替换为 `nobody_entire.mp4`,即可.

##  视频合并

`$echo -e "file nobody_00_30.mp4\r\nfile nobody.mp4" > list.txt`

`$ffmpeg -f concat -i list.txt -c copy concat.mp4`

##  播放速度

`$ffmpeg -i nobody_00_30.mp4 -vf "setpts=2*PTS" nobody_0.5rate_.mp4`

这样就将视频改为 `0.5` 倍速度了

