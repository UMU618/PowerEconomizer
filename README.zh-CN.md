# 超抠

简体中文 | [English](README.md)

众所周知，节能环保是稣的理念，清净优雅是稣的追求，低碳减排是稣的习惯。自从用上 MBP13 M1，稣就很喜欢它的安静和长续航，再加上工作一般用台式机，以至于稣的 Windows 笔记本基本都吃灰了。

最近，孩子们开始学编程，学校指定的是 Windows 系统，毕竟更普遍，所以稣考虑给 Windows 笔记本优化优化，改善它在节能方面上的弱势。

## 原理

俗话说：能力越大，功耗就越大。功耗随 CPU 频率的增高，而成指数级的增长！功耗越大，风扇的噪音也就越大！

对后台进程进行智能频率控制（降低），即可达到节能降噪的目的。

您肯定要担心性能了？不怕！[超抠](https://github.com/UMU618/PowerEconomizer)会实时监控前台进程，给它全频运行的能力。

节能就是牺牲后台进程的性能（反应速度）为代价的，但这个牺牲是值得的，因为用户一般只会注意和关心他们正在用的软件（前台进程）。再说，现代的 CPU 都太强了，即使降频，也基本够用。

其实，macOS 就是这么干的。

## 前提

- Windows 10 Insider Preview Build 21359 开始支持 [EcoQos](https://devblogs.microsoft.com/performance-diagnostics/introducing-ecoqos/)。（稣的开发测试环境是 Windows 11 22H2，不过 Windows 11 21H2 就能正常工作。）

- 大部分新的移动版处理器都支持，具体是：英特尔 10 代及以后,；AMD™ Ryzen™ 5000+；高通能装 ARM 版本 Windows 10+ 的都可以。

- 已有大佬用 C# 开发了基于 EcoQos 的 [EnergyStar](https://github.com/imbushuo/EnergyStar)，但它目前还不够好用。
