#include "OF/lib/PRTS/PrtsManager.hpp"
#include "OF/lib/Unit/Unit.hpp"
#include "zephyr/kernel.h"

[[noreturn]] int main()
{
    OF::StartUnits();
    OF::Prts::PrtsManager::initShell();
    while (true)
    {
        k_sleep(K_SECONDS(1U));
    }
}
