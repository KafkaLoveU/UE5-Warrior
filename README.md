# ⚔️ Warrior — UE5 GAS 第三人称 ARPG 战斗 Demo

> 基于 **Unreal Engine 5.4** + **Gameplay Ability System (GAS)** 的第三人称动作战斗项目。
> 纯 C++ 框架 + 蓝图技能，独立实现了从角色、技能、战斗判定到属性/UI 的完整战斗闭环。

---

## 🎮 玩法特性

| 能力 | 说明 |
|------|------|
| **移动 / 视角** | WASD 移动 + 鼠标转向（Enhanced Input）|
| **轻击 / 重击** | 斧头连击，动画通知驱动武器碰撞判定 |
| **翻滚** | 位移闪避，Motion Warping 驱动 |
| **格挡** | 格挡成功触发反击窗口 |
| **锁定目标** | 切换目标（左/右），镜头追踪 |
| **怒气系统** | 积攒怒气，怒气满可爆发 |
| **武器特殊技** | 斧头轻重特殊攻击（含跳劈）|
| **敌人 AI** | 行为树驱动，近战/远程/召唤多种敌人 |
| **拾取物** | 场景中拾取石头资源 |

---

## 🏗️ 技术架构

### 核心类设计（组件化 + 继承分层）

```
ACharacter (引擎)
└─ AWBaseCharacter            角色基类：装配 GAS 组件、初始化技能/属性
   ├─ AWHeroCharacter         玩家：输入绑定、相机、战斗/UI 组件
   └─ AWEnemyCharacter        敌人：AI 控制、碰撞盒攻击

UActorComponent
├─ UPawnCombatComponent       战斗核心：武器注册、碰撞开关、命中回调
│  ├─ UHeroCombatComponent    玩家战斗（伤害计算）
│  └─ UEnemyCombatComponent   敌人战斗
└─ UPawnUIComponent           UI 广播站（观察者模式）

UAbilitySystemComponent
└─ UWAbilitySystemComponent   技能调度：授予/激活/移除技能
```

### 设计模式应用

- **观察者模式**：属性变化 → UI 组件 `Broadcast` 委托 → 血条监听更新（玩家与 UI 完全解耦）
- **工厂模式**：`GiveAbility` 批量授予技能、GameMode `SpawnActor` 生成敌人，配置数据驱动
- **状态模式**：角色状态用 GameplayTag 标记（滚动/无敌/受击），代替 if-else
- **组件模式**：角色能力拆分为 Combat/UI/Ability 等组件挂载

---

## 🔄 GAS 技能完整链路

```
① 配置层（编辑器）
   UDataAsset_StartUpDataBase
   └─ FAbilitySet { InputTag, AbilityToGrant }

② 授予层（运行时）
   AWBaseCharacter::PossessedBy
   └─ UWAbilitySystemComponent::GrantHeroWeaponAbilities()
      └─ GiveAbility(Spec) → 技能进入 ActivatableAbilities

③ 输入层
   按键 → IA_xxx（Enhanced Input）→ InputTag

④ 激活层
   ASC::OnAbilityInputPressed(InputTag)
   └─ 遍历技能匹配标签 → TryActivateAbility
      └─ GameplayAbility 执行（动画 → 判定 → 效果）

⑤ 战斗判定
   动画 AnimNotify → PawnCombatComponent::ToggleWeaponCollision
   └─ 武器碰撞 → 命中回调 → 伤害计算

⑥ 属性/表现
   WAttributeSet 扣血 → 广播 UI 委托 → 血条更新
```

---

## 📁 目录结构

```
Source/Warrior/
├── Public/Private
│   ├── AbilitySystem/       # GAS 封装（ASC、AttributeSet、Abilities、Tasks）
│   ├── Characters/          # 角色（基类、玩家、敌人）
│   ├── Components/
│   │   ├── Combat/          # 战斗组件（武器注册、碰撞、伤害）
│   │   └── UI/              # UI 广播组件
│   ├── Controllers/         # 玩家/AI 控制器
│   ├── AnimInstances/       # 动画实例
│   ├── DataAssets/          # 输入配置、StartUp 数据资产
│   ├── GameMode/            # 游戏模式
│   ├── Interfaces/          # 解耦接口
│   └── Items/Weapons/       # 武器
Content/
└── Blueprints/              # 技能/角色/UI 蓝图（继承 C++ 类）
```

---

## 🚀 运行方法

### 环境要求

- Unreal Engine **5.4.x**
- Visual Studio 2022（含 C++ 工作负载）

### 步骤

```
1. 用 Epic Launcher 安装 UE 5.4
2. 安装插件：
   - FlatNodes（免费，FAB 商城）
   - ElectronicNodes（免费，FAB 商城）
   （仅蓝图编辑体验优化，缺失不影响编译运行）
3. 右键 Warrior.uproject → Generate Visual Studio project files
4. 双击 Warrior.uproject 打开，点击 Play
```

> ⚠️ 首次打开会自动编译 C++，需等待 5-15 分钟。

---

## 🛠️ 技术要点记录

- **UObject 反射**：UPROPERTY/UFUNCTION 驱动 GC、蓝图交互、序列化
- **GameplayTag**：技能/状态统一用标签标识，支持精确/模糊匹配激活
- **Enhanced Input**：InputAction + InputMappingContext 替代旧输入系统
- **数据驱动**：技能/属性由 DataAsset 配置，数值策划无需改代码

---

## 📌 说明

本项目为学生自学作品，用于学习 UE C++ 与 GAS 框架的实践练习。
