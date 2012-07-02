/*
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#include "msg.h"
#include "msg_internal.h"


extern volatile int numpros;

//
//Global MSG topology struct 
//
msg_topology_t msg_tcb;

static msg_topology_t children_topo[ENV_CORE_NUM];

static void
set_tcb_entry(msg_topology_t *topo, MSGI_NODE_STS_T reservation,
        uint32_t my_index, msg_addr_32_t ctx, msgi_cb_t *cb,
        msg_topology_t *parent, uint32_t num_peers,
        msg_topology_t *peers, uint32_t num_children,
        msg_topology_t *children, uint32_t max_processes,
        msgi_dmrQ_reg_t dmrQ_reg);

msg_topology_t *msgi_get_node(node_id_t rank, MSG_ERR_T *errcode)
{
    *errcode = MSG_SUCCESS;
    msg_topology_t *ret = NULL;

    return ret;
    //if()
}

//  =====   INTERNAL FUNCTIONS  =======
static void set_tcb_entry(msg_topology_t *topo, MSGI_NODE_STS_T reservation,
        uint32_t my_index, msg_addr_32_t ctx, msgi_cb_t *cb,
        msg_topology_t *parent, uint32_t num_peers,
        msg_topology_t *peers, uint32_t num_children,
        msg_topology_t *children, uint32_t max_processes,
        msgi_dmrQ_reg_t dmrQ_reg) 
{
    topo->reservation           =   reservation;
    topo->ctx                   =   ctx;
    topo->my_index              =   my_index;
    topo->cb                    =   cb; 
    topo->parent                =   parent;
    topo->num_peers             =   num_peers;
    topo->peers                 =   peers;
    topo->num_children          =   num_children;
    topo->children              =   children;
    topo->nproc_supported       =   max_processes;
    topo->nproc_active          =   0;  
    topo->dmrQ_reg              =   dmrQ_reg;

    topo->pids[ONLYPID].pid.pid_id     =   0;
    topo->pids[ONLYPID].status         =   MSGI_PID_INVALID;
    topo->pids[ONLYPID].nwaiters       =   0;
}

//  =====   CONSTRUCTOR and DESTRUCTOR  =======
MSG_ERR_T msgi_topology_init(void *argvp, void *envp)
{
    uint32_t  i                 =   0;
    MSG_ERR_T rc                =   MSG_SUCCESS;
    uint32_t  num_child_peers   =   0;
    //uint32_t  num_children      =   NUM_ACCS_PER_PROCESSOR;
    uint32_t  num_children      =   ENV_CORE_NUM - 1;

    //MSGMI_TOPO_LOCK();
    set_tcb_entry(  &msg_tcb,                   
            MSGI_NODE_RESERVED,     // TODO: implement the "node reserve" function             
            0,                      //  my_index    index of host is 0
            0,                      //  node_handle, ie., rank
            &msg_cb[0],            //  cb
            NULL,                   //  parent topo
            0,                      //  num_peers
            NULL,                   //  peer topos
            num_children,           //  number of childern
            &children_topo[0],      //  children topos
            MAX_PROCESSES_PER_NODE,
            MSGI_DMRQ_UNREGISTERED);

    msg_tcb.children = &children_topo[0];
    for (i = 0; i < msg_tcb.num_children; i++) {
        set_tcb_entry(  &msg_tcb.children[i],
                MSGI_NODE_RESERVED, // TODO: implement the "node reserve" function
                i + 1,
                0,
                &msg_cb[i + 1],
                &msg_tcb,
                num_child_peers,
                NULL,
                0,
                NULL,
                MAX_PROCESSES_PER_NODE,
                MSGI_DMRQ_UNREGISTERED);
    }
    //MSGMI_TOPO_UNLOCK();

    return rc;
}

MSG_ERR_T msgi_topology_exit(void)
{
    uint32_t i = 0;

    //MSGMI_TOPO_LOCK();
    // clean up children tcbs; set to values that will force an error if they
    //  are used again
    for (i = 0; i < msg_tcb.num_children; i++) {
        set_tcb_entry(  &msg_tcb.children[i],
                MSGI_NODE_INVALID,          // reservation
                i,                          // my_index
                i,                          // node handle
                &msg_cb[i + 1],            // cb
                &msg_tcb,                   // parent topo
                0,                          // num_peers
                NULL,                       // array of peers topos
                0,                          // leaf, nchildren = 0
                NULL,                       
                0,                          // num_processes
                MSGI_DMRQ_UNREGISTERED);
    }

    // clean up main tcb, set to value that will be force an error if it is
    //  uesd again
    set_tcb_entry(  &msg_tcb,
            MSGI_NODE_INVALID,      // reservation
            0,                      // my_index
            0,                      // node handle
            &msg_cb[0],            // cb
            NULL,                   // parent topo
            0,                      // num_peers
            NULL,                   // arrary of peers topos
            0,                      // number of children
            NULL,                   // array of children topos
            0,                      // num_processes
            MSGI_DMRQ_UNREGISTERED);

    //MSGMI_TOPO_UNLOCK();

    return MSG_SUCCESS;
}

