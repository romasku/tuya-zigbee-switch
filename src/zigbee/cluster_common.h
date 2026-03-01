#ifndef _CLUSTER_COMMON_H_
#define _CLUSTER_COMMON_H_

#define SETUP_ATTR_FOR_TABLE(table, attr_index, attr_id, attr_type, flag_val, attr_data) \
        table[attr_index].attribute_id = attr_id;                                        \
        table[attr_index].data_type_id = attr_type;                                      \
        table[attr_index].flag         = flag_val;                                       \
        table[attr_index].size         = (uint8_t)sizeof(attr_data);                     \
        table[attr_index].value        = (uint8_t *)&attr_data;

#define SETUP_ATTR(attr_index, attr_id, attr_type, flag_val, attr_data)                     \
        SETUP_ATTR_FOR_TABLE(cluster->attr_infos, attr_index, attr_id, attr_type, flag_val, \
                             attr_data)

#ifndef STRINGIFY
#define _STRINGIFY(x)                      #x
#define STRINGIFY(x)                       _STRINGIFY(x)
#endif

#define DEF_STR(string, name)              struct { uint8_t len; char str[sizeof(string) - 1]; \
} const name = { sizeof(string) - 1, string }
#define DEF_STR_NON_CONST(string, name)    struct { uint8_t len; char str[sizeof(string) - 1]; \
}       name = { sizeof(string) - 1, string }

#endif
