struct mw_api_work_list
{
	int size;
	void *works;
};

typedef struct mw_api_work_list mw_works;

struct mw_api_spec
{
    mw_works *(*create) (int argc, char **argv);
    /* create work: return a NULL-terminated list of work. Return NULL if it fails. */

    int (*result) (int sz, void *res);
    /* process result. Input is a collection of results, of size sz. Returns 1 on success, 0 on failure. */

    void *(*compute) (void *work);
    /* compute, returning NULL if there is no result, non-NULL if there is a result to be returned. */

    int work_sz, res_sz;
    /* size in bytes of the work structure and result structure, needed to send/receive messages */
};


void MW_Run (int argc, char **argv, struct mw_api_spec *f); /* run master-worker */