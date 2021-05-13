# Magic Block Game 解法
--------------------------

## 1. 游戏简介

1111111111

## 2. 求解最优解

`5 x 5` 的完整 `BFS`（深度优先搜索）：

```shell
depth = 26
cur.size() = 986011, next.size() = 1576893
visited.size() = 4142688

depth = 27
cur.size() = 1576893, next.size() = 2579722
visited.size() = 6722410

depth = 28
cur.size() = 2579722, next.size() = 4081269
visited.size() = 10803679

depth = 29
cur.size() = 4081269, next.size() = 6588822
visited.size() = 17392501

depth = 30
cur.size() = 6588822, next.size() = 10421089
visited.size() = 27813590

depth = 31
cur.size() = 10421089, next.size() = 16854052
visited.size() = 44667642

depth = 32
cur.size() = 16854052, next.size() = 26417351
visited.size() = 71084993

......

(内存使用量：已超过 11.x G Bytes)
```


