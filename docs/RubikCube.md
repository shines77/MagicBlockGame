# 魔方 - Rubik's Cube

## 1. 魔方总变化数

魔方别看只有 26 个小方块，变化可真是不少，魔方总的变化数为

魔方总变化数公式：

$$\frac{8! \times {3}^{8} \times 12! \times {2}^{12}}{3 \times 2 \times 2} = 43,252,003,274,489,856,000$$

或者约等于 $4.3 \times {10}^{19}$ 。如果你一秒可以转 3 下魔方，不计重复，你也需要转 4542 亿年，才可以转出魔方所有的变化，这个数字是目前估算宇宙年龄的大约 30 倍。如果你感兴趣魔方总变化数的道理，[请到这里看看](http://www.rubik.com.cn/group.htm) -- [http://www.rubik.com.cn/group.htm](http://www.rubik.com.cn/group.htm)。

* 三阶魔方的入门教程

    [http://www.rubik.com.cn/beginner.htm](http://www.rubik.com.cn/beginner.htm)

    ```java
    首先我们来介绍一下魔方的构造。

    三阶魔方有 `8` 个角色块，`12` 个棱色块，`6` 个中心块，中心块相对位置永远不变，一定是 `红橙` 相对，`蓝绿` 相对，`黄白` 相对，也就是相近的颜色相对。中心块是什么颜色，这一面最后就会是什么颜色。大家注意黄白中心块永远是相对的，我们第一步就要用到这个。
    ```

## 2. 魔方算法

### 2.1 Two phase algorithm

* Cube Explorer -- Kociemba

    [http://kociemba.org/cube.htm](http://kociemba.org/cube.htm)

* 高效的计算机解魔方算法：二阶段算法 -- [知乎]
  
    [https://zhuanlan.zhihu.com/p/73054362](https://zhuanlan.zhihu.com/p/73054362)

* 三阶魔方还原 two phase 算法--学习笔记
  
    [https://blog.csdn.net/harryhare/article/details/36894373](https://blog.csdn.net/harryhare/article/details/36894373)

* 【ACM】魔方 -- 11题

    [https://www.cnblogs.com/bombe1013/p/5631184.html](https://www.cnblogs.com/bombe1013/p/5631184.html)

* 【poj.org】POJ 3701

    [http://poj.org/problem?id=3701](http://poj.org/problem?id=3701)

    ```java
    三阶魔方搜索，算法还是双广。但是这题wa了好几次，一定要注意读题，首先面1表示FRONT，同样旋转方式都是基于此前提下的，展开方式也是。因此，需要重新映射一下。这个题目不要使用状态等价，可以直接用longlong表示状态，因为254足够了，这里的1的个数一定为9。代码使用G++可以790ms过，C++一直超时。
    ```

* 数字华容道 3×3，最后剩下 7 和 8 颠倒，解法？

    [https://www.zhihu.com/question/279495025](https://www.zhihu.com/question/279495025)

    ```java
    回答一：

        无解。原因可能是你用的软件是直接随机生成八个数字的位置而不是随机生成打乱的过程。这是明显的错误做法。

    回答二：

        解不了的。
        要对调得至少两次。
        和普通三阶魔方装反一个棱、对调一对棱不能还原是一个道理。
    ```

* 数字华容道有何解法？

    [https://www.zhihu.com/question/265397001](https://www.zhihu.com/question/265397001)

    里面有 4x4 数字华容道解题顺序 和 数字华容道 3×3 解题方式教学（视频，作者：喵喵Limo）。

* 十六格拼图 — IDA * 算法

    [https://www.cnblogs.com/gao79135/p/14066131.html](https://www.cnblogs.com/gao79135/p/14066131.html)

    有对 IDA * 算法的详细描述，以及一段示例代码，并配上了博主自己写的详细注释，注释文字非常的长，见解也是初级水平，但非常的详细。我给整理了一下，把注释截短了，详见 “`ida_star.c`” 。

* 搜索算法 — IDA ∗

    [https://blog.csdn.net/weixin_43501684/article/details/100858305](https://blog.csdn.net/weixin_43501684/article/details/100858305)

    主要内容：

    ```java
    找到最优解

        * 总的来说迭代加深搜索类似于广搜的思想，但是空间上和深度优先搜索相当。适用于深度没有明确上限的情况。

        * 实际操作中，要求进行剪枝，因为深搜的时间开销很大，即使限制了深度，还需要在无解的情况下及时退出。怎么做呢？需要使用一个估价函数（和A*中的相当）

    估价函数：

    $$ f(n) = g(n) + h(n) $$

    其中，g(n) 表示现在已经走的步数， h(n) 表示从当前到达最优解的步数的一个估计。** h(n) 一定要小于等于到达最优解的一个实际的值）**，如果 f(n) > maxdep ，即大于当前设定的最大深度，说明当前深度限制下找不到解，直接退出即可。

    总的来说，IDA * 是在限制搜索深度下，加入估价函数进行剪枝，然后不断迭代加深的一种算法，算法保证找到答案时，搜索深度是最小的，部分情况下可以代替广搜。题目一定要保证有解，否则会无限递归下去。
    ```

* [AI] A * 算法

    [https://blog.csdn.net/potato_uncle/article/details/109125856](https://blog.csdn.net/potato_uncle/article/details/109125856)

    主要介绍了 `A *` 算法，同时还提了一下 `IDA *`，递归最佳优先搜索 `RBFS`，`SMA *` 算法等。

* [ML] 无信息搜索 Uninformed Search

    [https://blog.csdn.net/potato_uncle/article/details/108957342](https://blog.csdn.net/potato_uncle/article/details/108957342)

    ```shell
    一、广度优先搜索 Breadth-First Search

    二、一致代价搜索 Uniform-Cost Search

    三、深度优先搜索 Depth-First Search

    四、深度受限搜索 Depth-limited search

    五、迭代深度搜索 Iterative Deepening Search

    六、双向搜索 Bi-directional search

    七、无信息搜索算法性能总结
    ```

## X. 3D魔方程序

* HTML5交互式3D魔方游戏代码

    [https://sc.chinaz.com/jiaobendemo.aspx?downloadid=25201885445763](https://sc.chinaz.com/jiaobendemo.aspx?downloadid=25201885445763)

* [4399] 游戏：3D超级魔方

    [http://www.4399.com/flash/207064_1.htm](http://www.4399.com/flash/207064_1.htm)

## Y. 参考文献

* 用 LaTeX 写数学公式

    [https://www.jianshu.com/p/e6d2368e451a](https://www.jianshu.com/p/e6d2368e451a)

* Latex数学公式编写

    [https://www.cnblogs.com/endlesscoding/p/9797237.html](https://www.cnblogs.com/endlesscoding/p/9797237.html)
