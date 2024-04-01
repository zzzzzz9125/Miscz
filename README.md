# MisczTools for Vegas Pro
## 介绍
一个为 Vegas Pro 提供像素图像相关功能的工具合集，由 **MisczOFX** 和 **MisczTools 脚本** 这两部分组成。

<br>MisczOFX 由 [openfx-misc](https://github.com/NatronGitHub/openfx-misc) 修改而来，是可用于 Vegas Pro 的 OFX 插件。改版修改了原插件的部分效果，并精简成为 Mz_TransformOFX、Mz_PositionOFX、Mz_Card3DOFX、Mz_CornerPinOFX、Mz_SpriteSheet 这五个 FX，可以实现像素图形的硬边缘放大变换、Sprite 表播放等功能。MisczTools 脚本 v.1.2.0 以前的版本完全基于 MisczOFX，提供像素放大和 Sprite 表播放的一键脚本操作。

<br>从 v.1.2.0 开始，MisczTools 脚本不再依赖于 MisczOFX，而是转而使用 [FFmpeg](https://ffmpeg.org/download.html) 逻辑进行重渲染操作。

目前，MisczTools 脚本包含两个脚本：PixelScalingTool 脚本 和 SpriteSheetTool 脚本。

[PixelScalingTool 脚本](https://github.com/zzzzzz9125/Miscz/blob/main/Scripts/PixelScalingTool.cs) 提供对原素材的一键像素放大功能，而 [SpriteSheetTool 脚本](https://github.com/zzzzzz9125/Miscz/blob/main/Scripts/SpriteSheetTool.cs) 提供对 Sprite 表的切割、播放、重渲染等功能。

使用教程详见：https://www.bilibili.com/read/cv33279086 。

<br>

## 安装说明

**注意，从 v.1.2.0 开始，由于 MisczTools 脚本不再依赖于 MisczOFX，因此 MisczOFX 的版本号仍停留在 v.1.1.0 版，并且暂时不打算有更新意向。**

**现在如果你想使用 v.1.2.0 及以上的新版 MisczTools 脚本，你需要提前将 [FFmpeg](https://ffmpeg.org/download.html) 添加至环境变量中。方法请自行百度。**

文件下载可转到最新的 [Releases](https://github.com/zzzzzz9125/Miscz/releases/)。中国国内替代下载链接：https://share.weiyun.com/BFrp1QKx 。

<br>

### 脚本安装
自动安装：下载对应的版本的 .exe 安装包，安装。Vegas Pro 14 及以上，请下载 Magix 版，Vegas Pro 13 及以下，请下载 Sony 版。

手动安装：下载 `MisczTools_v.1.2.x.zip` 并解压。将脚本文件放置于对应文件夹中。推荐使用以下两个路径：

Magix Vegas Pro: `C:\ProgramData\Vegas Pro\Script Menu\`

Sony Vegas Pro: `C:\ProgramData\Sony\Vegas Pro\Script Menu\`

<br>

若无效，请尝试以下路径：（17.0 = 你的 Vegas 版本号）

`C:\ProgramData\Vegas Pro\17.0\Script Menu\`

`C:\Users\<username\Documents\Vegas Script Menu\`

`C:\Users\<username\AppData\Local\Vegas Pro\17.0\Script Menu\`

`C:\Users\<username\AppData\Roaming\Vegas Pro\17.0\Script Menu\`

`C:\Users\<username\AppData\Local\Vegas Pro\Script Menu\`

`C:\Users\<username\AppData\Roaming\Vegas Pro\Script Menu\`

如果上述文件夹不存在，请手动创建。

<br>

### MisczOFX 安装
自动安装：下载 `MisczOFX_v.1.1.0_Chinese.exe` 并运行，解压到默认路径即可。

手动安装：下载 `MisczTools_v.1.2.x.zip` 并解压，将 `Miscz.ofx.bundle` 放置到以下路径： `C:\Program Files\Common Files\OFX\Plugins\`

如果上述文件夹不存在，请手动创建。

<br>

---

# MisczTools for Vegas Pro
## Introduction
A collection of tools for Vegas Pro to provide pixel image related features, consisting of two parts: **MisczOFX** and **MisczTools Scripts**.

<br>MisczOFX, modified by [openfx-misc](https://github.com/NatronGitHub/openfx-misc), is an OFX plug-in available for Vegas Pro. The revision modified part of the effects of the original plug-in, and simplified into 5 FX: Mz_TransformOFX, Mz_PositionOFX, Mz_Card3DOFX, Mz_CornerPinOFX and Mz_SpriteSheet. They can realize the functions of scaling and transformation of pixel graphics and Sprite Sheets playback. Previous versions (older than v.1.2.0) of the MisczTools Scripts were based entirely on MisczOFX, providing one-click script operations for Pixel Scaling and Sprite Sheets Playback.

<br>Starting with v.1.2.0, MisczTools Scripts no longer depend on MisczOFX, but instead use [FFmpeg](https://ffmpeg.org/download.html) logic for rendering.

Currently, the MisczTools Scripts consist of two scripts: PixelScalingTool Script and SpriteSheetTool Script.

[PixelScalingTool Script](https://github.com/zzzzzz9125/Miscz/blob/main/Scripts/PixelScalingTool.cs) provides a one-click way to scale Pixel Art materials, [SpriteSheetTool Script](https://github.com/zzzzzz9125/Miscz/blob/main/Scripts/SpriteSheetTool.cs) provides cutting, playback, rerendering and other functions for Sprite Sheets.

See this tutorial: https://www.bilibili.com/read/cv33279086 (Chinese Only sorry, I will write an English ver. sometime).

<br>

## Installation instructions

**Note: Starting with v.1.2.0, since MisczTools Scripts no longer depend on MisczOFX, the version of MisczOFX remains at v.1.1.0 and is not intended to be updated for the time being.**

**Now if you want to use MisczTools Scripts (v.1.2.0 or later version), you must add [FFmpeg](https://ffmpeg.org/download.html) to the environment variables in advance. Google it if you don't know how to.**

You can go to the [Latest Releases](https://github.com/zzzzzz9125/Miscz/releases/) for file downloads.

<br>

### Scripts Install
Auto Install: Download the .exe file you need and install it. For Vegas Pro 14 or above, please download Magix version. For Vegas Pro 13 or below, please download Sony version.

Manual Install: Download `MisczTools_v.1.2.x.zip`, decompress it, and then put the script files into the following path. These two paths are recommended:

Magix Vegas Pro: `C:\ProgramData\Vegas Pro\Script Menu\`

Sony Vegas Pro: `C:\ProgramData\Sony\Vegas Pro\Script Menu\`

<br>

If this doesn't work, try: (17.0 = your Vegas Pro version number)

`C:\ProgramData\Vegas Pro\17.0\Script Menu\`

`C:\Users\<username\Documents\Vegas Script Menu\`

`C:\Users\<username\AppData\Local\Vegas Pro\17.0\Script Menu\`

`C:\Users\<username\AppData\Roaming\Vegas Pro\17.0\Script Menu\`

`C:\Users\<username\AppData\Local\Vegas Pro\Script Menu\`

`C:\Users\<username\AppData\Roaming\Vegas Pro\Script Menu\`

If no folder exists, create one, then put the files in.

<br>

### MisczOFX Install
Auto Install: Download and run `MisczOFX_v.1.1.0_English.exe`, decompress it to the default path.

Manual Install: Download `MisczTools_v.1.2.x.zip`, decompress it. For English users, delete `Miscz.ofx.bundle\Contents\Resources\Miscz.xml` first. Then put the `Miscz.ofx.bundle` into the following path: `C:\Program Files\Common Files\OFX\Plugins\`

If no folder exists, create one, then put the files in.
