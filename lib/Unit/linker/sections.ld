SECTION_DATA_PROLOGUE(unit_registry,,)
{
    __start_unit_registry = .;
    KEEP(*(SORT_BY_NAME(.unit_registry)));
    __stop_unit_registry = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)