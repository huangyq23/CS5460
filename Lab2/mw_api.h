struct mw_works
{
    int size;
    void *works;
};

typedef struct mw_works mw_works;

typedef mw_works *(*mw_create_func) (int argc, char **argv, void *meta);
typedef int (*mw_result_func) (int sz, void *res, void *meta);
typedef void *(*mw_compute_func) (void *work);

struct mw_api_spec
{
    mw_create_func create;
    mw_result_func result;
    mw_compute_func compute;
    int work_sz;
    int res_sz;
    int meta_sz;
};

typedef struct mw_api_spec mw_api_spec;

void MW_Run (int argc, char **argv, mw_api_spec *f);