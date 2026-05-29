# DifferentialDrive 参考文献与工程资料

这个文件集中保存 `SUPP.md` 中用到的参考资料。它们不是要求我们把小船完全当作轮式机器人，而是为“双马达差速控制、速度/转向混控、平滑控制、安全超时”这些设计提供依据。

## 论文资料

0. Chunyue Li, Jiajia Jiang, Fajie Duan 等人，2019  
   *Modeling and Experimental Testing of an Unmanned Surface Vehicle with Rudderless Double Thrusters*.  
   作用：这是目前最贴近我们小船的参考。论文对象是“无舵、双推进器水面无人船”，它明确说明：两个推进器速度共同控制船速，左右推进器差速控制转向；直线运动需要左右推进器速度相同，差速为零。  
   对我们的意义：强力支持“保留左右马达独立控制能力”，也支持在中间层使用“共同速度 common mode + 差速 differential mode”的设计。  
   注意：它证明的是双推进器差速控制适合无舵船，不是严格证明“网页双杆 UI 永远最优”。  
   链接：  
   - https://pmc.ncbi.nlm.nih.gov/articles/PMC6539673/
   - https://www.mdpi.com/1424-8220/19/9/2051

1. Kanayama, Y., Kimura, Y., Miyazaki, F., Noguchi, T.  
   *A stable tracking control method for an autonomous mobile robot*, IEEE International Conference on Robotics and Automation, 1990.  
   作用：支持“不要让输入直接变成马达瞬时输出”，而应使用速度目标、平滑和稳定跟踪控制思想。  
   链接：  
   - https://cir.nii.ac.jp/crid/1573387449091026048
   - https://doi.org/10.1109/robot.1990.126006

2. Doria-Cerezo 等人  
   *Sliding mode control of a differential-drive mobile robot following a path*.  
   作用：说明差速模型里“左右执行器平均值决定前进趋势，左右执行器差值决定转向趋势”。  
   链接：  
   - https://upcommons.upc.edu/bitstreams/992a7d05-d152-48b4-9396-69bf95f5e797/download

3. Zhang 等人，2024  
   *Universal Trajectory Optimization Framework for Differential Drive Robot Class*.  
   作用：说明差速驱动机器人仍然是现代移动机器人常见模型，适合作为双执行器控制架构参考。  
   链接：  
   - https://arxiv.org/abs/2409.07924

4. Isleyen 等人  
   *Feedback Motion Prediction for Safe Unicycle Robot Navigation*.  
   作用：说明 differential drive robot 可以抽象成 unicycle 模型，用于安全导航和运动预测。  
   链接：  
   - https://arxiv.org/abs/2209.12648

## 工程资料

1. WPILib Drive Classes  
   作用：提供成熟机器人项目里的 `Tank Drive`、`Arcade Drive`、`Curvature Drive` 工程设计参考。  
   链接：  
   - https://docs.wpilib.org/en/stable/docs/software/hardware-apis/motors/wpi-drive-classes.html

2. WPILib `DifferentialDrive` API  
   作用：展示成熟库如何组织差速驱动接口，以及如何处理不同驾驶模式。  
   链接：  
   - https://github.wpilib.org/allwpilib/docs/release/cpp/classfrc_1_1_differential_drive.html

## 对我们小船的结论

这些资料支持的是控制架构，而不是水面动力学的完全等价证明。我们的小船仍然需要实测调参，因为水面阻力、螺旋桨负载、电池电压和左右马达差异都会影响实际表现。
