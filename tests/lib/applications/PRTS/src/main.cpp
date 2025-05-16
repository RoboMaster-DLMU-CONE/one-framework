#include "OF/lib/applications/PRTS/PrtsManager.hpp"
#include "OF/lib/applications/Unit/Unit.hpp"
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
