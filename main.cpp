#include "BankManager.h"

int main()
{
    BankManager bank;
    
    bank.boot_up_scanner();
    
    bank.show_main_menu();
    
    return 0;
}