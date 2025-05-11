#ifndef UNITTHREADMANAGER_HPP
#define UNITTHREADMANAGER_HPP

#include <memory>
#include <vector>

#include "Unit.hpp"

namespace OF
{
    class UnitThreadManager
    {
    public:
        static void initializeThreads(const std::vector<std::unique_ptr<Unit>>& units);

    private:
        static void createThreadForUnit(Unit* unit, const UnitInfo& info);

        static void threadEntryFunction(void* unit, void*, void*);
    };
} // namespace OF


#endif // UNITTHREADMANAGER_HPP
