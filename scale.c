#include <stdio.h>
#include <string.h>
int main()
{
    for (;;)
    {
        char num[128] = {0};
        int bin[128] = {0};
        printf("Hex to Bin:");
        scanf("%s",num);
        getchar();
        int i, j, len = strlen(num);
        long long n = 0;
        for (i = ('0' == num[0] && 'x' == num[1])?2:0; i < len; i++)
        {
            if (num[i] <= '9') num[i] -= 48;
            else
            {
                if (num[i] >= 'a') num[i] -= 32;
                num[i] -= 55;
            }
            n = 16*n + num[i];
        }
        printf("DEC: %lld\nBIN: ",n);
        i = 0;
        while (n)
        {
            bin[i++] = n%2;
            n /= 2;
        }
        for (j = i - 1; j >= 0; j--) printf("%d",bin[j]);
        putchar('\n');
    }
    return 0;
}
