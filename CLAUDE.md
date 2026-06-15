# CLAUDE.md — 贪吃蛇项目规范

## VPN 代理

推送 GitHub 时使用以下代理：
```
https://liangxin.xyz/api/v1/liangxin?OwO=69e4848f0615aacc306b8017fdb1c36b
```
本地代理端口 `127.0.0.1:7897` 也可用。

## 项目仓库

https://github.com/DarthSea/BUPT_Eating_Snake_Graphics

## 开发规范

1. **代码审查**：写完代码后反复检查，防止出现 bug
2. **修改范围**：只修改用户指定的部分，其他部分如需修改必须先询问用户同意
3. **禁止擅自修改**：不要自作聪明擅自修改任何地方，任何改动都要经过用户同意
4. **文档同步**：任务完成无误后，自动更新以下文档：
   - `docs/聊天记录与项目上下文汇总.md` — 记录本次会话改动摘要
   - `docs/图形化贪吃蛇概要设计说明书.md` — 更新模块说明和版本记录
   - `docs/项目技术栈与实现原理.md` — 课堂分享用，新技术或模块变更时更新
5. **推送**：文档更新后提交并推送到 GitHub 仓库

## 编译命令

```powershell
& 'E:\VS\MSBuild\Current\Bin\MSBuild.exe' BUPT_Eating_Snake_Graphics.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
```

## 技术栈

- C 语言
- EasyX 图形库
- Visual Studio 2022 (MSVC)
- XInput (手柄支持)
