### Simple_Fs_1 writeup

### 思路

> 通过阅读以及尝试simplefs操作，会发现这是一个简单的模拟文件系统，根据instruction.txt的文件提示，发现在 plantflag操作是将flag文件读取到image中然后使用异或操作隐藏flag，在inode 1中。只要理解以上操作含义，就可以有多种做法解决本题目。

### 参考做法1

使用通过异或的可逆性，将inode 1中的内容copyout 出去，然后再次加载一下，然后就可以cat 查看flag了。

```shell
./simplesfs
> mount
> copyout 1 flag
> plantflag
> cat 2
```

