#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

const char lklklklkljj = '\001';
const char STX = '\002', ETX = '\003';
char ret[60] = {0x3, 0x6a, 0x6d, 0x47, 0x6e, 0x5f, 0x3d, 0x75, 0x61, 0x53, 0x5a, 0x4c, 0x76, 0x4e, 0x34, 0x77, 0x46, 0x78, 0x45, 0x36, 0x52, 0x2b, 0x70, 0x2, 0x44, 0x32, 0x71, 0x56, 0x31, 0x43, 0x42, 0x54, 0x63, 0x6b};
char tests[60]; // = "cwNG1paBu=6Vn2kxSCqm+_4LETvFRZDj";
char r[60];
int LEN = 34;
char *ss, *str;
char **table;

int 
__libc_longjmp1 (sigjmp_buf env, int val,int xxx)
{
  /* Perform any cleanups needed by the frames being unwound.  */
  _longjmp_unwind (env, val);

  if (env[0].__mask_was_saved)
    /* Restore the saved signal mask.  */
    (void) __sigprocmask (SIG_SETMASK,
			  (sigset_t *) &env[0].__saved_mask,
			  (sigset_t *) NULL);

  /* Call the machine-dependent function to restore machine state
     without shadow stack.  */
  __longjmp_cancel (env[0].__jmpbuf, val ?: 1);
  return -1;
}

jmp_buf bufferA, bufferB, bufferC;

static int sfi_memcmp(const void *s1, const void *s2, int len)
{
    unsigned char *p = s1;
    unsigned char *q = s2;
    int charCompareStatus = 0;
    // If both pointer pointing same memory block
    if (s1 == s2)
    {
        return charCompareStatus;
    }
    while (len > 0)
    {
        if (*p != *q)
        { // compare the mismatching character
            charCompareStatus = (*p > *q) ? 1 : -1;
            break;
        }
        len--;
        p++;
        q++;
    }
    return charCompareStatus;
}

static int strcmp2(const char *p1, const char *p2) // (const char) *p1 表示: 读取p1指向的类型的值,接着转换为const char. 如果p1是double*,则将读取double所有字节的内容,接着转换为const char. [*p1也表示完整的将实参传了进来]
{
    const unsigned char *s1 = (const unsigned char *)p1; // 初始化一个常量无符号字符指针s1, 为了字符方便比较, 全都转换为无符号常量char指针.
    const unsigned char *s2 = (const unsigned char *)p2; // (const unsigned char *) p2 表示: 首先指针指向为p2, 然后读取p2中指向位置的第一个字节内容. 即使是double*也只读取第一个字节.
    unsigned char c1, c2;
    do
    {
        c1 = (unsigned char)*s1++; // 指针后移一位sizeof(char),也就是1字节到后一个字符处.
        c2 = (unsigned char)*s2++;
        if (c1 == '\0') // 结束循环: 字符指针移动到s1字符串的结尾处'\0', 如果此时的c2还有字符时,相减为负; 如果此时的c2恰好也是'\0',相减为0.
            return c1 - c2;
    } while (c1 == c2); // 循环条件: 通过字符指针s1和s2的自增自减,移动到的字符都是一样的.
    return c1 - c2;     // 结束循环: 字符指针移动到s2字符串的结尾处'\0', 而此时的c1还有字符,相减为正.
}

static int compareStrings(const void *a, const void *b)
{
    char *aa = *(char **)a;
    char *bb = *(char **)b;
    return strcmp2(aa, bb);
}

// #define SWAP(a, b, size)             \
//     do                               \
//     {                                \
//         size_t __size = (size);      \
//         char *__a = (a), *__b = (b); \
//         do                           \
//         {                            \
//             char __tmp = *__a;       \
//             *__a++ = *__b;           \
//             *__b++ = __tmp;          \
//         } while (--__size > 0);      \
//     } while (0)

static void SWAP(char *a, char *b, int size)
{
    size_t __size = (size);
    char *__a = (a), *__b = (b);
    do
    {
        char __tmp = *__a;
        *__a++ = *__b;
        *__b++ = __tmp;
    } while (--__size > 0);
    return;
}

#define MAX_THRESH 4

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

static void
sfi_qsort(void *const pbase)
{

    /*
    sfi_qsort实际上是通过调用这个_quicksort实现的。最后的arg是个workaround，用来搞定
    sfi_qsort_r的。暂且不去理会。下面是一些准备工作：
    */
   int size =8;

    char *base_ptr = (char *)pbase;

    const size_t max_thresh = MAX_THRESH * 8;
    size_t total_elems = 34;



    /*
    明确了对内存操作是以sizeof(char)为单位的。max_thresh实际上是改用插入排序时，维护
    当前区间的两个指针（类型均为char*）之间的距离。另外，如果元素数量为0，sfi_qsort啥也不
    干，函数直接返回。
    */
   int (*cmp)(const void *, const void *) = &compareStrings;

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

    {
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

void setionA()
{
    if (strchr(tests, STX) || strchr(tests, ETX))
    {
        __libc_longjmp1(bufferA, 1,0);
    }
    int jmp_ret = setjmp(bufferB);
    if (jmp_ret == 0)
    {
        __libc_longjmp1(bufferA, 2,1);
    }
    sprintf(ss, "%c%s%c", STX, tests, ETX);
    jmp_ret = setjmp(bufferB);
    if (jmp_ret == 0)
        __libc_longjmp1(bufferA, 1,2);
    if (jmp_ret >= 1)
    {
        str = calloc(LEN + 1, sizeof(char));
        strcpy(str, ss + jmp_ret - 1);
        if (jmp_ret - 1 > 0)
            strncat(str, ss, jmp_ret - 1);
        table[jmp_ret - 1] = str;
    }
    __libc_longjmp1(bufferA, jmp_ret,3);
}

void setionB()
{
    int jmp_ret = setjmp(bufferC);
    if (jmp_ret == 0)
    {
        __libc_longjmp1(bufferA, 1,1);
    }
    else if (jmp_ret < LEN + 1)
    {
        r[jmp_ret - 1] = table[jmp_ret - 1][LEN - 1];
        free(table[jmp_ret - 1]);
        __libc_longjmp1(bufferA, jmp_ret + 1,3);
    }
    if (sfi_memcmp(r, ret, LEN) == 0)
        __libc_longjmp1(bufferA, 1,4);
    else
        __libc_longjmp1(bufferA, 2,0x60);
}

int main()
{
    gets(tests);
    int i;
    int jmp_ret = setjmp(bufferA);
    if (jmp_ret == 0)
    {
        setionA();
    }
    else if (jmp_ret == 1)
    {
        return 1;
    }
    ss = calloc(LEN + 1, sizeof(char));
    jmp_ret = setjmp(bufferA);
    if (jmp_ret == 0)
        __libc_longjmp1(bufferB, 1,0x64);
    table = malloc(LEN * sizeof(const char *));

    jmp_ret = setjmp(bufferA);
    if (jmp_ret == 0)
        __libc_longjmp1(bufferB, 1,0x65);
    else if (jmp_ret < LEN + 1)
    {
        __libc_longjmp1(bufferB, jmp_ret + 1,0x63);
    }
    sfi_qsort(table);
    jmp_ret = setjmp(bufferA);
    if (jmp_ret == 0)
        setionB();
    else if (jmp_ret < LEN + 1)
    {
        __libc_longjmp1(bufferC, jmp_ret,0x62);
    }
    free(table);
    free(ss);
    jmp_ret = setjmp(bufferA);
    if (jmp_ret == 0)
        __libc_longjmp1(bufferC, LEN + 1,0x61);
    if (jmp_ret == 1)
        printf("*CTF{%s}\n", tests);
    return 0;
}
