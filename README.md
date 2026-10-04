# ⚔️ Warrior — UE5 GAS 第三人称 ARPG 战斗 Demo

> 基于 **Unreal Engine 5.4** + **Gameplay Ability System (GAS)** 的第三人称动作战斗项目。
> 纯 C++ 框架 + 蓝图技能，独立实现了从角色、技能、战斗判定到属性/UI 的完整战斗闭环。

---

## 🎬 演示展示

| 战斗演示 | 背包展示 |
|----------|----------|
| ![战斗演示](Media/combat-demo.gif) | ![背包展示](Media/inventory-demo.gif) |

- **战斗演示**：连击 / 翻滚闪避 / 格挡反击 / 锁定目标 / 敌人 AI 行为树
- **背包展示**：石头资源拾取、物品栏 UI 与格子拖拽

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
| **跳跃** | 蓝图技能 GA_Hero_Jump，走通「加标签 → 配输入 → 建技能类 → 注册数据资产」流程 |
| **敌人 AI** | 行为树驱动，近战/远程/召唤多种敌人，残血会脱离战斗逃跑 |
| **拾取物** | 场景中拾取石头资源 |
| **主菜单 / 暂停菜单** | 开始游戏、选项、退出；游戏内暂停、返回主菜单、退出 |

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

## 🤖 敌人 AI（行为树）

```
Blackboard: BB_Enemy_Base
├─ TargetActor / DistToTarget     目标 Actor 与距离
├─ HealthPercent                  血量百分比（自定义 Service 写入）
└─ RetreatLocation                逃跑坐标（自定义 Service 写入）

Behavior Tree: BT_Glacer / BT_Guardian
Root
└─ Service: Update Health Percent + Calculate Retreat Location
   └─ Selector
      ├─ 近战分支      DistToTarget ≤ 250
      ├─ 远程分支      DistToTarget ≥ 650
      ├─ 找射击位分支  EQS 查询射击位置
      └─ 残血逃跑分支  HealthPercent < 0.3（Observer Aborts = Both）
```

两个用 C++ 扩展的自定义 `UBTService`：

| 类 | 职责 |
|------|------|
| `UBTService_UpdateHealthPercent` | 每 0.2s 从 ASC 取 `UWAttributeSet`，算出 `当前血量 / 最大血量` 写入黑板 |
| `UBTService_CalculateRetreatLocation` | 按 `自身位置 - 目标位置` 求反方向，乘以逃跑距离，算出逃跑点写入黑板 |

> **踩坑记录（均为实际排查过的问题）**
> - `UBTService` 构造函数**必须**调用 `INIT_SERVICE_NODE_NOTIFY_FLAGS()`，否则引擎不会调用 `TickNode()`，服务挂上去毫无效果（`BTTask` 对应 `INIT_TASK_NODE_NOTIFY_FLAGS()`）。
> - `FBlackboardKeySelector` 必须在 `InitializeFromAsset()` 里调用 `ResolveSelectedKey(*BBAsset)` 才会生效。
> - `Interval` 不能设 0：逐帧写黑板会让带 `Observer Aborts` 的装饰器每帧重算、引起行为树抖动，本项目取 0.2s。
> - 血量必须从 `ASC->GetSet<UWAttributeSet>()` 取：基类成员指针持有的那个实例并没有被 ASC 注册，GE 只作用于 ASC 内的实例，用成员指针会读到恒定 1.0 的假数据。

---

## ♻️ 对象池（投掷物）

```
UWProjectilePoolSubsystem : UWorldSubsystem
├─ TMap<TSubclassOf<AActor>, FActorPool>   按精确子类分桶，池与池互不干扰
│    └─ FActorPool { AllActors, FreeActors }
├─ AcquireProjectile()   取：优先复用 Free 队列，池空才 Spawn，超过 MaxPoolSize 则丢弃
└─ ReleaseProjectile()   还：停组件、关碰撞、隐藏，并挪到地图外远点

IProjectilePoolableInterface（蓝图 / C++ 双实现协议）
├─ OnAcquiredFromPool()    重置伤害 Spec、Transform、碰撞、Niagara 与寿命定时器
├─ OnReleasedToPool()      停表现、清状态
├─ OnRemovedFromPool()     池被销毁时真正 Destroy
└─ GetPoolableProjectileClass()

UWProjectilePoolStatics : UBlueprintFunctionLibrary
├─ Spawn Projectile From Pool   （引脚与 SpawnActor 对齐，蓝图可直接换节点）
└─ Return Projectile To Pool
```

**关键改造点**

- 敌方火球由 `SpawnActor` / `Destroy` 改为 `Acquire` / `Release`，消除高频弹幕反复创建销毁 Actor 带来的 GC 压力。
- 禁用 `InitialLifeSpan`：引擎到期会直接 `Destroy()`，绕过回池逻辑；改用自管 `FTimerHandle`，到期调 `ReturnToPool()`。
- 复用实例时必须 `Reset()` 命中记录，否则第二发飞出去会跳过打过的同一目标。
- 接口方法必须标 `UFUNCTION(BlueprintNativeEvent)`，UHT 才会生成 `Execute_xxx` 桥接函数与 `xxx_Implementation` 虚函数（纯 C++ 虚函数两者都不生成，会编译报错）。

---

## 🖥️ UI 与游戏流程

```
MainMenuMap（GameDefaultMap）
└─ WB_MainMenu : UWUserWidgetBase
   ├─ StartGame  → 加载战斗关卡
   ├─ Options    → WB_OptionsMenu
   └─ Quit       → QuitGame

暂停菜单 WB_PauseScreen : UWUserWidgetBase
├─ Back       → SetGamePaused(false)
├─ MainMenu   → OpenLevel(MainMenuMap)
└─ Quit       → QuitGame
```

- 关卡用 `GameplayTag`（`GameData.Level.*`）在 `UWGameInstance` 里登记，按标签取 `TSoftObjectPtr<UWorld>`；切图时用 `MoviePlayer` 显示加载屏。
- 暂停菜单由 `IA_PauseMenu`（Enhanced Input）触发，`UWUserWidgetBase` 作为 C++ 基类统一 UMG 与 C++ 的交互入口。

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
│   ├── AI/                  # 自定义行为树节点（BTService）
│   ├── Controllers/         # 玩家/AI 控制器
│   ├── AnimInstances/       # 动画实例
│   ├── DataAssets/          # 输入配置、StartUp 数据资产
│   ├── GameMode/            # 游戏模式
│   ├── Interfaces/          # 解耦接口（含对象池协议 ProjectilePoolableInterface）
│   ├── Subsystems/          # 世界子系统（投掷物对象池）
│   ├── Widgets/             # UMG 基类
│   ├── WarriorTypes/        # 蓝图函数库（对象池蓝图入口）
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
> 启动后进入主菜单（MainMenuMap），点「开始游戏」进入战斗关卡。

---

## 🛠️ 技术要点记录

- **UObject 反射**：UPROPERTY/UFUNCTION 驱动 GC、蓝图交互、序列化
- **GameplayTag**：技能/状态统一用标签标识，支持精确/模糊匹配激活
- **Enhanced Input**：InputAction + InputMappingContext 替代旧输入系统
- **数据驱动**：技能/属性由 DataAsset 配置，数值策划无需改代码
- **行为树 / 黑板**：AI 决策与数据解耦，用 C++ 扩展 BTService 把运行时数据（血量、逃跑点）喂给黑板
- **UWorldSubsystem**：随 World 创建与销毁，作为对象池的生命周期容器，关卡切换时自动回收
- **对象池模式**：高频生成销毁的对象（投掷物）走池化复用，用接口协议解耦池与具体对象类型

---

## 📌 说明

本项目为学生自学作品，用于学习 UE C++ 与 GAS 框架的实践练习。
