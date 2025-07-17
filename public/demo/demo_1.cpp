#include "../_public.h"
using namespace idc;

int main(int argc, char *argv[])
{
    cout << "demo_1" << endl;

    char str_1[512];

    strcpy(str_1,"name:messi,no:10,job:striker.");
    bool result = replace_str(str_1,":","", true);
    cout << "result: " << result << endl;
    printf("str_1: %s\n", str_1);

    // strcpy(str, "abcdefgabc");
    // replace_str(str, "abc", "def", true);
    // cout << str << endl;

    // strcpy(str, "abcdefgabc");
    // replace_str(str, "abc", "babc", true);
    // cout << str << endl;

    return 0; 
}