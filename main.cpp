#include "BankManager.h"
#include <ctime>
#include <cstdlib>

int main()
{
    srand(time(0));
    BankManager bank;
    
    bank.boot_up_scanner();
    
    bank.show_main_menu();
    
    return 0;
}