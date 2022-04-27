#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char STX = '\002', ETX = '\003';

int compareStrings(const void *a, const void *b)
{
    char *aa = *(char **)a;
    char *bb = *(char **)b;
    return strcmp(aa, bb);
}


/*
首先实现sfi_qsort需要有一个swap：
*/

#define SWAP(a, b, size)             \
    do                               \
    {                                \
        size_t __size = (size);      \
        char *__a = (a), *__b = (b); \
        do                           \
        {                            \
            char __tmp = *__a;       \
            *__a++ = *__b;           \
            *__b++ = __tmp;          \
        } while (--__size > 0);      \
    } while (0)

/*
为了阅读方便，我把用于宏定义续行用的\去掉了，方便看到语法高亮。

SWAP的定义很简单，只是以char为单位长度进行交换。实际上，对于足够大的对象，这里还有
一定的优化空间，但sfi_qsort不能假定你的对象足够大。这也是sfi_qsort统计性能不如C++的
std::sort的原因之一。

所谓sfi_qsort并不是单纯的快速排序，当快速排序划分区间小到一定程度时，改用插入排序可以在
更少的耗时内完成排序。glibc将这个小区间定义为元素数不超过常数4的区间：
*/

#define MAX_THRESH 4

/*
可以认为对每一个小区间的插入排序耗时是不超过一个常数值（即对4个元素进行插入排序的最
差情况）的，这样插入排序总耗时也可以认为是线性的，不影响总体时间复杂度。

接下来这段不难理解：
*/

typedef struct
{
    char *lo;
    char *hi;
} stack_node;
// typedef unsigned int size_t; 
#define CHAR_BIT 8

#define STACK_SIZE (CHAR_BIT * sizeof(size_t))
#define PUSH(low, high) ((void)((top->lo = (low)), (top->hi = (high)), ++top))
#define POP(low, high) ((void)(--top, (low = top->lo), (high = top->hi)))
#define STACK_NOT_EMPTY (stack < top)

/*
这显然是要手动维护栈来模拟递归，避免实际递归的函数调用开销了。题主自己的实现在元素数
量过多的时候会崩溃，估计就是递归过深爆栈了。

比较值得一提的是STACK_SIZE的选择。由于元素数量是由一个size_t表示的，size_t的最大
值应该是2 ^ (CHAR_BIT * sizeof(size_t))，快速排序的理想“递归”深度是对元素总数
求对数，所以理想的“递归”深度是CHAR_BIT * sizeof(size_t)。虽然理论上最差情况下
“递归”深度会变成2 ^ (CHAR_BIT * sizeof(size_t))，但是一方面我们不需要从头到尾
快速排序（区间足够小时改用插入排序），另一方面，我们是在假设元素数量等于size_t的上
限...这都把内存给挤满了=_=所以最后glibc决定直接采用理想“递归”深度作为栈大小上限。

接下来该进入sfi_qsort的正文了：
*/

static void
sfi_qsort(void *const pbase, size_t total_elems, size_t size,
      int (*cmp)(const void *, const void *))
{

    /*
    sfi_qsort实际上是通过调用这个_quicksort实现的。最后的arg是个workaround，用来搞定
    sfi_qsort_r的。暂且不去理会。下面是一些准备工作：
    */

    char *base_ptr = (char *)pbase;

    const size_t max_thresh = MAX_THRESH * size;

    if (total_elems == 0)
        return;

    /*
    明确了对内存操作是以sizeof(char)为单位的。max_thresh实际上是改用插入排序时，维护
    当前区间的两个指针（类型均为char*）之间的距离。另外，如果元素数量为0，sfi_qsort啥也不
    干，函数直接返回。
    */

    if (total_elems > MAX_THRESH)
    // 如果元素数大于终止区间（4个元素），则进行快速排序
    {
        char *lo = base_ptr;
        char *hi = &lo[size * (total_elems - 1)];
        stack_node stack[STACK_SIZE];
        stack_node *top = stack;

        PUSH(NULL, NULL);

        /*
        开始维护“递归”栈
        */

        while (STACK_NOT_EMPTY)
        {
            char *left_ptr;
            char *right_ptr;

            /*
            这里采用的是中值划分，用于降低特定序列导致递归恶化影响。也就是说，对区间里的首元
            素、中间元素和尾元素先进行排序。排序方式是...呃，是冒泡。反正也就3个元素嘛。
            */

            char *mid = lo + size * ((hi - lo) / size >> 1);

            if ((*cmp)((void *)mid, (void *)lo) < 0)
                SWAP(mid, lo, size);
            if ((*cmp)((void *)hi, (void *)mid) < 0)
                SWAP(mid, hi, size);
            else
                goto jump_over;
            if ((*cmp)((void *)mid, (void *)lo) < 0)
                SWAP(mid, lo, size);
        jump_over:;

            /*
            接下来就要进行快排划分了：
            */

            left_ptr = lo + size;
            right_ptr = hi - size;

            do
            {
                while ((*cmp)((void *)left_ptr, (void *)mid) < 0)
                    left_ptr += size;

                while ((*cmp)((void *)mid, (void *)right_ptr) < 0)
                    right_ptr -= size;

                /*
                和传统的划分方式不同，首先把中间当作键值在左侧找到一个不小于mid的元素，右侧找到一个
                不大于mid的元素
                */

                if (left_ptr < right_ptr)
                {
                    SWAP(left_ptr, right_ptr, size);
                    if (mid == left_ptr)
                        mid = right_ptr;
                    else if (mid == right_ptr)
                        mid = left_ptr;
                    left_ptr += size;
                    right_ptr -= size;
                }

                /*
                如果左右侧找到的不是同一个元素，那就交换之。如果左右侧任意一侧已经达到mid，就把mid往
                另一边挪。因为键值已经被“丢”过去了。
                */

                else if (left_ptr == right_ptr)
                {
                    left_ptr += size;
                    right_ptr -= size;
                    break;
                }
            }

            /*
            如果左右侧是同一元素，划分其实已经大功告成了。
            */

            while (left_ptr <= right_ptr);

            /*
            接下来，将划分出来的所有大于的终止区间的区间压入栈准备下一次划分，连这里都会发生丧心
            病狂的优化...不能浪费已经在栈里的原区间上下界=_=
            */

            if ((size_t)(right_ptr - lo) <= max_thresh)
            {
                if ((size_t)(hi - left_ptr) <= max_thresh)
                    POP(lo, hi);
                else
                    lo = left_ptr;
            }
            else if ((size_t)(hi - left_ptr) <= max_thresh)
                hi = right_ptr;
            else if ((right_ptr - lo) > (hi - left_ptr))
            {
                PUSH(lo, right_ptr);
                lo = left_ptr;
            }
            else
            {
                PUSH(left_ptr, hi);
                hi = right_ptr;
            }
        }
    }

    /*
    至此快速排序工作结束。
    */

#define min(x, y) ((x) < (y) ? (x) : (y))
    // 头一回学宏“函数”都应该见过这货吧

    /*
    接下来是插入排序
    */

    {

        /*
        众所周知，传统的插入排序中每一趟插入（假设是向前插入），我们都会迭代有序区间，如果已
        经迭代到了区间头部，那就把该元素插在区间首部；否则如果前一个元素不大于待插入元素，则
        插在该元素之后。

        这种插入方式会导致每一次迭代要进行两次比较：一次比较当前迭代位置与区间首部，一次比较
        两个元素大小。但事实上，我们可以用一个小技巧省掉前一次比较————插入排序开始前，先将容
        器内最小元素放到容器首部，这样就可以保证每趟插入你永远不会迭代到区间首部，因为你总能
        在中途找到一个不小于自己的元素。

        注意，如果区间大于终止区间，搜索最小元素时我们不必遍历整个区间，因为快速排序保证最小
        元素一定被划分到了第一个终止区间，也就是头4个元素之内。
        */

        char *const end_ptr = &base_ptr[size * (total_elems - 1)];
        char *tmp_ptr = base_ptr;
        char *thresh = min(end_ptr, base_ptr + max_thresh);
        char *run_ptr;

        for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
            if ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
                tmp_ptr = run_ptr;

        if (tmp_ptr != base_ptr)
            SWAP(tmp_ptr, base_ptr, size);

        run_ptr = base_ptr + size;
        while ((run_ptr += size) <= end_ptr)
        {
            tmp_ptr = run_ptr - size;
            while ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
                tmp_ptr -= size;

            tmp_ptr += size;
            if (tmp_ptr != run_ptr)
            {
                char *trav;

                trav = run_ptr + size;
                while (--trav >= run_ptr)
                {
                    char c = *trav;
                    char *hi, *lo;

                    for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
                        *hi = *lo;
                    *hi = c;
                }
            }
        }
    }
}


int bwt(const char *s, char r[])
{
    int i, len = strlen(s) + 2;
    char *ss, *str;
    char **table;
    if (strchr(s, STX) || strchr(s, ETX))
        return 1;
    ss = calloc(len + 1, sizeof(char));
    sprintf(ss, "%c%s%c", STX, s, ETX);
    table = malloc(len * sizeof(const char *));
    for (i = 0; i < len; ++i)
    {
        str = calloc(len + 1, sizeof(char));
        strcpy(str, ss + i);
        if (i > 0)
            strncat(str, ss, i);
        table[i] = str;
    }
    sfi_qsort(table, len, sizeof(const char *), compareStrings);
    for (i = 0; i < len; ++i)
    {
        r[i] = table[i][len - 1];
        free(table[i]);
    }
    free(table);
    free(ss);
    return 0;
}

void ibwt(const char *r, char s[])
{
    int i, j, len = strlen(r);
    char **table = malloc(len * sizeof(const char *));
    for (i = 0; i < len; ++i)
        table[i] = calloc(len + 1, sizeof(char));
    for (i = 0; i < len; ++i)
    {
        for (j = 0; j < len; ++j)
        {
            memmove(table[j] + 1, table[j], len);
            table[j][0] = r[j];
        }
        sfi_qsort(table, len, sizeof(const char *), compareStrings);
    }
    for (i = 0; i < len; ++i)
    {
        if (table[i][len - 1] == ETX)
        {
            strncpy(s, table[i] + 1, len - 2);
            break;
        }
    }
    for (i = 0; i < len; ++i)
        free(table[i]);
    free(table);
}

void makePrintable(const char *s, char t[])
{
    strcpy(t, s);
    for (; *t != '\0'; ++t)
    {
        if (*t == STX)
            *t = '^';
        else if (*t == ETX)
            *t = '|';
    }
}



int main()
{
    int i, res, len;
    char *r, *s;
    char tests[40] = "cwNG1paBu=6Vn2kxSCqm+_4LETvFRZDj";
    char ret[40] = {0x3,0x6a,0x6d,0x47,0x6e,0x5f,0x3d,0x75,0x61,0x53,0x5a,0x4c,0x76,0x4e,0x34,0x77,0x46,0x78,0x45,0x36,0x52,0x2b,0x70,0x2,0x44,0x32,0x71,0x56,0x31,0x43,0x42,0x54,0x63,0x6b};


    len = strlen(tests);
    printf(" --> ");
    r = calloc(len + 3, sizeof(char));
    res = bwt(tests, r);
    if (res == 1)
    {
        printf("ERROR: String can't contain STX or ETX\n");
    }else{
        for(i = 0;i<len+2;i++){
            printf("0x%x,",r[i]);
        }
        puts(r);
    }
    if(strncmp(r,ret,len+2)==0){
        printf("*CTF{%s}\n",tests);
    }
    
    s = calloc(len + 1, sizeof(char));
    ibwt(r, s);
    printf(" --> %s\n", s);
    free(r);
    free(s);

    return 0;
}