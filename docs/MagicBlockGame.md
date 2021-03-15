# Magic Block Game 解法
--------------------------

## 1. 游戏简介

1111111111

## 2. 求解最优解

`5 x 5` 的完整 `BFS`（深度优先搜索）：

```shell
depth = 23
cur.size() = 318076, next.size() = 514329
visited.size() = 1265542

depth = 24
cur.size() = 514329, next.size() = 901562
visited.size() = 2167104

depth = 25
cur.size() = 901562, next.size() = 1466812
visited.size() = 3633916

depth = 26
cur.size() = 1466812, next.size() = 2610325
visited.size() = 6244241

depth = 27
cur.size() = 2610325, next.size() = 4188748
visited.size() = 10432989

depth = 28
cur.size() = 4188748, next.size() = 7300204
visited.size() = 17733193

depth = 29
cur.size() = 7300204, next.size() = 11755206
visited.size() = 29488399

depth = 30
cur.size() = 11755206, next.size() = 20702707
visited.size() = 50191106

depth = 31
cur.size() = 20702707, next.size() = 32928421
visited.size() = 83119527

......

(内存使用量：超过 11.x G Bytes)
```


