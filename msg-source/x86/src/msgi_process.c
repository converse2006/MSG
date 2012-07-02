/* 
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#include "msg_internal.h"

extern int numpros;

//  =====   CONSTRUCTOR and DESTRUCTOR  =======
MSG_ERR_T msgi_process_init(int *local_rank)
{
        int i,pid;
        for(i=0;i<numpros-1;i++)
        {
            pid = fork();
            switch(pid)
            {
                case -1:
                    break;
                case 0:
                    *local_rank = i+1;
                    return MSG_SUCCESS;
                        break;
                default:
                    break;
            }
        }
        *local_rank = 0;

        return MSG_SUCCESS;
}


MSG_ERR_T msgi_process_exit()
{
    MSG_ERR_T rc = MSG_SUCCESS;

    return rc;
}
