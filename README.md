# s1mpleFps

> UE5.4 C++ 多人竞技 FPS — 个人实习求职项目

## 技术栈

`Unreal Engine 5.4.4` `C++` `Steam OnlineSubsystem` `Enhanced Input` `Behavior Tree` `EQS` `UMG`

## 功能概览

### 🎮 角色系统
- 第一/第三人称切换
- ADS 瞄准、蹲伏、疾跑
- Enhanced Input 全键位绑定
- 脚步声 AI 感知噪音

### 🔫 武器系统
- 3 武器槽 + 切换（1/2/3/滚轮）
- 半自动 / 连发 / 全自动 三种射击模式
- 后坐力 + 散布模拟
- 换弹、拾取地上武器
- 枪口火焰特效（Multicast 网络同步）

### 💣 手雷系统
- 破片手雷（径向伤害 + 穿透 + 摄像机震动）
- 闪光弹（距离衰减 + 视角遮挡计算）
- 烟雾弹（粒子特效 + 持续时间）
- 高抛/低抛 + 拉栓烹饪
- 圆环 HUD 进度条

### 🛡️ 伤害系统
- HitZone 部位检测（头部/身体/四肢 伤害倍率）
- 护甲系统（多护甲叠加 + 减伤计算）
- 穿透值抵消护甲机制
- DataAsset 数据驱动配置

### 🤖 PvE AI 系统
- 行为树 + EQS 环境查询
- 4 级警戒状态（Calm → Suspicious → Alert → Combat）
- 视觉 / 听觉 / 受伤 三种感知
- 小队协调（目标共享、角色分配：Assault/Suppressor/Flanker）
- 掩体寻找、撤退、换弹 AI 行为
- 死亡重生机制

### 🌐 PvP 网络
- Listen Server + Steam 联机
- GameInstance Session 管理（Host/Find/Join）
- 15+ RPC（Server/Client/Multicast）
- 属性复制 + OnRep 回调
- 客户端预测（切枪/拾取/购买） + 服务端验证 + 回滚
- Late Joiner 支持

### 📊 比赛系统
- 热身倒计时 → 正式比赛 → 加时赛 → Sudden Death
- 击杀/死亡/分数 实时统计
- 击杀播报 KillFeed
- 计分板（Tab 键，按分数排序）
- 经济系统（击杀/连杀奖励 + 连死补偿）
- CS 风格购买菜单（武器/护甲/手雷/医疗）
- 自动复活 + 随机出生点
- 比赛结算 UI

### 💬 聊天系统
- 全体聊天（Y 键）+ 队聊（U 键）
- TeamId 过滤
- 消息气泡自动消失

### 🩹 治疗系统
- 医疗物品使用（H 键）
- 引导时间 + 减速惩罚
- HUD 治疗倒计时显示

## 架构

```
Source/s1mpleFps/
├── s1mpleFpsCharacter    — 玩家角色（输入/武器/手雷/治疗）
├── TP_WeaponComponent    — 武器组件（射击/换弹/ADS）
├── DamageComponent       — 共享伤害组件（HitZone/护甲/穿透）
├── s1mpleFpsGameMode     — 单人 PvE 模式
├── s1mpleFpsPvPGameMode  — 多人 PvP 模式
├── s1mpleFpsGameState    — 比赛状态（复制）
├── s1mpleFpsPlayerState  — 玩家数据（复制）
├── s1mpleFpsPlayerController — 玩家控制器（HUD/购买/聊天）
├── s1mpleFpsGameInstance — Steam Session 管理
├── GrenadeComponent      — 手雷组件（库存/拉栓/投掷）
├── GrenadeProjectile     — 手雷基类
│   ├── _Frag             — 破片手雷
│   ├── _Flash            — 闪光弹
│   └── _Smoke            — 烟雾弹
├── EnemyAIController     — AI 控制器（行为树/感知/小队）
├── EnemyCharacter        — AI 角色
├── HUDWidget             — HUD UI
├── DataAssets/           — 数据驱动配置
│   ├── WeaponData        — 武器参数
│   ├── GrenadeData       — 手雷参数
│   ├── ArmorData         — 护甲参数
│   └── HealthData        — 治疗参数
└── BehaviorTree/         — AI 行为树节点
    ├── Tasks（Fire/Reload/Investigate/FindCover/Retreat）
    ├── Decorators（AIState/Ammo/Target）
    └── Services（Fire/AlertValue/Blackboard）
```

## 构建 & 运行

### 环境要求
- Unreal Engine 5.4.4
- Visual Studio 2022
- Steam（多人联机需要）

### 构建步骤
```bash
# 1. 克隆仓库
git clone <your-repo-url>
cd s1mpleFps

# 2. 复制配置文件
cp Config/DefaultEngine.ini.template Config/DefaultEngine.ini

# 3. 从 Epic 商城下载免费资产（项目依赖，未纳入 Git）
#    打开 Epic Games Launcher → 虚幻引擎 → 商城 → 免费内容：
#    - Starter Content（UE5 自带，项目模板已包含）
#    - Paragon: Murdock（免费角色）
#    - Stylized Log Cabin（免费场景）

# 4. 生成 VS 项目文件
右键 s1mpleFps.uproject → Generate Visual Studio project files

# 5. 编译 & 运行
打开 s1mpleFps.sln → Build Solution
```

### 多人测试
```bash
# 主机：编辑器中 Play as Listen Server
# 客户端：编辑器中 Play as Client，或在另一台电脑上直连 IP
```

## 操作说明

| 按键 | 功能 |
|------|------|
| WASD | 移动 |
| 鼠标 | 视角 |
| 左键 | 开火 |
| 右键 | ADS 瞄准 |
| R | 换弹 |
| 1/2/3 | 切换武器槽 |
| 滚轮 | 上一把/下一把武器 |
| G | 装备/取消手雷 |
| 左键(手雷模式) | 高抛 |
| 右键(手雷模式) | 低抛 |
| Q(手雷模式) | 切换手雷类型 |
| E | 拾取武器 |
| H | 使用医疗物品 |
| Ctrl | 蹲伏 |
| 空格 | 跳跃 |
| V | 第一/第三人称切换 |
| Tab | 计分板 |
| B | 购买菜单（热身阶段） |
| Y | 全体聊天 |
| U | 队聊 |
| Esc | 暂停 |

## 技术亮点

- **客户端-服务器权威架构**：服务端验证所有关键操作（开火/购买/拾取），客户端乐观预测 + 回滚
- **DataAsset 数据驱动**：武器/手雷/护甲/治疗全部通过 PrimaryDataAsset 配置，无需改代码
- **AI 全栈实现**：从 AIPerception 到 BehaviorTree 到 EQS，完整的 UE5 AI 管线
- **多播同步策略**：UI 数据走 OnRep，视觉效果走 Multicast RPC，敏感操作走 Server RPC
- **组件化设计**：DamageComponent / GrenadeComponent / WeaponComponent 可复用

## 开发日志

项目历经 6 次迭代，从 UE5 模板逐步构建为完整多人 FPS：
1. 基础 FPS 角色 + 武器系统
2. PvE AI 行为树 + EQS + 小队协调
3. PvP 网络复制 + RPC + 客户端预测
4. 比赛系统（热身/加时/Sudden Death）
5. 购买系统 + 聊天 + Session 管理
6. 手雷系统 + HUD 完善 + Bug 修复

## License

个人学习项目，仅供参考。
