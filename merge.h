#ifndef malena_merge_h
#define malena_merge_h

/* 64 bytes */
struct TempInvertHead {
    size_t termid_num;
    size_t others[7];
};

struct InvertHead {
    size_t termid_num;
    size_t others[7];
};

int merge_temp_inverts(char **temp_inv_paths, size_t size,
        const char *invert_dir);
#endif
