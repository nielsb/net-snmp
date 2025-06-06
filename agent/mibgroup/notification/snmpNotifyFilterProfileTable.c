/*
 * This file was generated by mib2c and is intended for use as
 * a mib module for the ucd-snmp snmpd agent. 
 *
 * Portions of this file are copyrighted by:
 * Copyright (c) 2016 VMware, Inc. All rights reserved.
 * Use is subject to license terms specified in the COPYING file
 * distributed with the Net-SNMP package.
 */


/*
 * This should always be included first before anything else 
 */
#include <net-snmp/net-snmp-config.h>

#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif


/*
 * minimal include directives 
 */
#include <net-snmp/net-snmp-features.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "header_complex.h"
#include "snmpNotifyTable_data.h"
#include "snmpNotifyFilterProfileTable.h"

#ifndef NETSNMP_NO_WRITE_SUPPORT
netsnmp_feature_require(header_complex_find_entry);
#endif /* NETSNMP_NO_WRITE_SUPPORT */

/*
 * snmpNotifyFilterProfileTable_variables_oid:
 *   this is the top level oid that we want to register under.  This
 *   is essentially a prefix, with the suffix appearing in the
 *   variable below.
 */


oid             snmpNotifyFilterProfileTable_variables_oid[] =
    { 1, 3, 6, 1, 6, 3, 13, 1, 2 };
#ifndef NETSNMP_NO_WRITE_SUPPORT
static const size_t table_offset =
    OID_LENGTH(snmpNotifyFilterProfileTable_variables_oid) + 3 - 1;
#endif

/*
 * variable2 snmpNotifyFilterProfileTable_variables:
 *   this variable defines function callbacks and type return information 
 *   for the snmpNotifyFilterProfileTable mib section 
 */


struct variable2 snmpNotifyFilterProfileTable_variables[] = {
    /*
     * magic number        , variable type , ro/rw , callback fn  , L, oidsuffix 
     */
#define   SNMPNOTIFYFILTERPROFILENAME  3
    {SNMPNOTIFYFILTERPROFILENAME, ASN_OCTET_STR, NETSNMP_OLDAPI_RWRITE,
     var_snmpNotifyFilterProfileTable, 2, {1, 1}},
#define   SNMPNOTIFYFILTERPROFILESTORTYPE  4
    {SNMPNOTIFYFILTERPROFILESTORTYPE, ASN_INTEGER, NETSNMP_OLDAPI_RWRITE,
     var_snmpNotifyFilterProfileTable, 2, {1, 2}},
#define   SNMPNOTIFYFILTERPROFILEROWSTATUS  5
    {SNMPNOTIFYFILTERPROFILEROWSTATUS, ASN_INTEGER, NETSNMP_OLDAPI_RWRITE,
     var_snmpNotifyFilterProfileTable, 2, {1, 3}},

};
/*
 * (L = length of the oidsuffix) 
 */


/*
 * global storage of our data, saved in and configured by header_complex() 
 */



/*
 * init_snmpNotifyFilterProfileTable():
 *   Initialization routine.  This is called when the agent starts up.
 *   At a minimum, registration of your variables should take place here.
 */
void
init_snmpNotifyFilterProfileTable(void)
{
    DEBUGMSGTL(("snmpNotifyFilterProfileTable", "initializing...  "));

    init_snmpNotifyFilterProfileTable_data();

    /*
     * register ourselves with the agent to handle our mib tree 
     */
    REGISTER_MIB("snmpNotifyFilterProfileTable",
                 snmpNotifyFilterProfileTable_variables, variable2,
                 snmpNotifyFilterProfileTable_variables_oid);


    /*
     * place any other initialization junk you need here 
     */


    DEBUGMSGTL(("snmpNotifyFilterProfileTable", "done.\n"));
}

void
shutdown_snmpNotifyFilterProfileTable(void)
{
    shutdown_snmpNotifyFilterProfileTable_data();
}


/*
 * var_snmpNotifyFilterProfileTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for var_snmpNotifyFilterProfileTable above.
 */
unsigned char  *
var_snmpNotifyFilterProfileTable(struct variable *vp,
                                 oid * name,
                                 size_t * length,
                                 int exact,
                                 size_t * var_len,
                                 WriteMethod ** write_method)
{


    struct snmpNotifyFilterProfileTable_data *StorageTmp = NULL;
    int found = 1;


    DEBUGMSGTL(("snmpNotifyFilterProfileTable",
                "var_snmpNotifyFilterProfileTable: Entering...  \n"));
    /*
     * this assumes you have registered all your data properly
     */
    StorageTmp = snmpNotifyFilterProfileTable_oldapi_find(vp, name, length,
                                                          exact, var_len,
                                                          write_method);
    if (StorageTmp == NULL)
        found = 0;

    switch (vp->magic) {
#ifndef NETSNMP_NO_WRITE_SUPPORT
    case SNMPNOTIFYFILTERPROFILENAME:
        *write_method = write_snmpNotifyFilterProfileName;
        break;

    case SNMPNOTIFYFILTERPROFILESTORTYPE:
        *write_method = write_snmpNotifyFilterProfileStorType;
        break;

    case SNMPNOTIFYFILTERPROFILEROWSTATUS:
        *write_method = write_snmpNotifyFilterProfileRowStatus;
        break;
#endif /* !NETSNMP_NO_WRITE_SUPPORT */
    default:
        *write_method = NULL;
    }

    if (!found) {
        return NULL;
    }

    /*
     * this is where we do the value assignments for the mib results.
     */
    switch (vp->magic) {

    case SNMPNOTIFYFILTERPROFILENAME:
        *var_len = StorageTmp->snmpNotifyFilterProfileNameLen;
        return (u_char *) StorageTmp->snmpNotifyFilterProfileName;

    case SNMPNOTIFYFILTERPROFILESTORTYPE:
        *var_len = sizeof(StorageTmp->snmpNotifyFilterProfileStorType);
        return (u_char *) & StorageTmp->snmpNotifyFilterProfileStorType;

    case SNMPNOTIFYFILTERPROFILEROWSTATUS:
        *var_len = sizeof(StorageTmp->snmpNotifyFilterProfileRowStatus);
        return (u_char *) & StorageTmp->snmpNotifyFilterProfileRowStatus;


    default:
        ERROR_MSG("");
    }

    return NULL;
}

#ifndef NETSNMP_NO_WRITE_SUPPORT

static struct snmpNotifyFilterProfileTable_data *StorageNew;

int
write_snmpNotifyFilterProfileName(int action,
                                  u_char * var_val,
                                  u_char var_val_type,
                                  size_t var_val_len,
                                  u_char * statP,
                                  oid * name, size_t name_len)
{
    static char    *tmpvar;
    struct snmpNotifyFilterProfileTable_data *StorageTmp = NULL;
    static size_t   tmplen;
    size_t          newlen = name_len - table_offset;


    DEBUGMSGTL(("snmpNotifyFilterProfileTable",
                "write_snmpNotifyFilterProfileName entering action=%d...  \n",
                action));
    if (action != RESERVE1 &&
        (StorageTmp = snmpNotifyFilterProfileTable_oldapi_find(
            NULL, &name[table_offset], &newlen, 1, NULL, NULL)) == NULL) {
        if ((StorageTmp = StorageNew) == NULL)
            return SNMP_ERR_NOSUCHNAME;     /* remove if you support creation here */
    }


    switch (action) {
    case RESERVE1:
        if (var_val_type != ASN_OCTET_STR) {
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len < 1 || var_val_len > NOTIFY_NAME_MAX) {
            return SNMP_ERR_WRONGLENGTH;
        }
        break;


    case RESERVE2:
        /*
         * memory reseveration, final preparation... 
         */
        tmpvar = StorageTmp->snmpNotifyFilterProfileName;
        tmplen = StorageTmp->snmpNotifyFilterProfileNameLen;
        StorageTmp->snmpNotifyFilterProfileName = calloc(1, var_val_len + 1);
        if (NULL == StorageTmp->snmpNotifyFilterProfileName)
            return SNMP_ERR_RESOURCEUNAVAILABLE;
        break;


    case FREE:
        /*
         * Release any resources that have been allocated 
         */
        break;


    case ACTION:
        /*
         * The variable has been stored in string for
         * you to use, and you have just been asked to do something with
         * it.  Note that anything done here must be reversable in the UNDO case 
         */
        memcpy(StorageTmp->snmpNotifyFilterProfileName, var_val, var_val_len);
        StorageTmp->snmpNotifyFilterProfileNameLen = var_val_len;
        break;


    case UNDO:
        /*
         * Back out any changes made in the ACTION case 
         */
        SNMP_FREE(StorageTmp->snmpNotifyFilterProfileName);
        StorageTmp->snmpNotifyFilterProfileName = tmpvar;
        StorageTmp->snmpNotifyFilterProfileNameLen = tmplen;
        break;


    case COMMIT:
        /*
         * Things are working well, so it's now safe to make the change
         * permanently.  Make sure that anything done here can't fail! 
         */
        SNMP_FREE(tmpvar);
        snmp_store_needed(NULL);
        break;
    }
    return SNMP_ERR_NOERROR;
}



int
write_snmpNotifyFilterProfileStorType(int action,
                                      u_char * var_val,
                                      u_char var_val_type,
                                      size_t var_val_len,
                                      u_char * statP,
                                      oid * name, size_t name_len)
{
    static int      tmpvar;
    long            value = *((long *) var_val);
    struct snmpNotifyFilterProfileTable_data *StorageTmp = NULL;
    size_t          newlen = name_len - table_offset;


    DEBUGMSGTL(("snmpNotifyFilterProfileTable",
                "write_snmpNotifyFilterProfileStorType entering action=%d...  \n",
                action));
    if (action != RESERVE1 &&
        (StorageTmp = snmpNotifyFilterProfileTable_oldapi_find(
            NULL, &name[table_offset], &newlen, 1, NULL, NULL)) == NULL) {
        if ((StorageTmp = StorageNew) == NULL)
            return SNMP_ERR_NOSUCHNAME;     /* remove if you support creation here */
    }

    switch (action) {
    case RESERVE1:
        if (var_val_type != ASN_INTEGER) {
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len != sizeof(long)) {
            return SNMP_ERR_WRONGLENGTH;
        }
        if (value != SNMP_STORAGE_OTHER && value != SNMP_STORAGE_VOLATILE
            && value != SNMP_STORAGE_NONVOLATILE) {
            return SNMP_ERR_WRONGVALUE;
        }
        break;


    case RESERVE2:
        /*
         * memory reseveration, final preparation... 
         */
        break;


    case FREE:
        /*
         * Release any resources that have been allocated 
         */
        break;


    case ACTION:
        /*
         * The variable has been stored in long_ret for
         * you to use, and you have just been asked to do something with
         * it.  Note that anything done here must be reversable in the UNDO case 
         */
        tmpvar = StorageTmp->snmpNotifyFilterProfileStorType;
        StorageTmp->snmpNotifyFilterProfileStorType = *((long *) var_val);
        break;


    case UNDO:
        /*
         * Back out any changes made in the ACTION case 
         */
        StorageTmp->snmpNotifyFilterProfileStorType = tmpvar;
        break;


    case COMMIT:
        /*
         * Things are working well, so it's now safe to make the change
         * permanently.  Make sure that anything done here can't fail! 
         */
        snmp_store_needed(NULL);
        break;
    }
    return SNMP_ERR_NOERROR;
}






int
write_snmpNotifyFilterProfileRowStatus(int action,
                                       u_char * var_val,
                                       u_char var_val_type,
                                       size_t var_val_len,
                                       u_char * statP,
                                       oid * name, size_t name_len)
{
    struct snmpNotifyFilterProfileTable_data *StorageTmp = NULL;
    static struct snmpNotifyFilterProfileTable_data *StorageDel;
    size_t          newlen = name_len - table_offset;
    static int      old_value;
    int             set_value = *((long *) var_val);
    netsnmp_variable_list *vars;


    DEBUGMSGTL(("snmpNotifyFilterProfileTable",
                "write_snmpNotifyFilterProfileRowStatus entering action=%d...  \n",
                action));
    StorageTmp = snmpNotifyFilterProfileTable_oldapi_find(
        NULL, &name[table_offset], &newlen, 1, NULL, NULL);

    switch (action) {
    case RESERVE1:
        if (var_val_type != ASN_INTEGER || var_val == NULL) {
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len != sizeof(long)) {
            return SNMP_ERR_WRONGLENGTH;
        }
        if (set_value < 1 || set_value > 6 || set_value == RS_NOTREADY) {
            return SNMP_ERR_WRONGVALUE;
        }
        /*
         * stage one: test validity 
         */
        if (StorageTmp == NULL) {
            /*
             * create the row now? 
             */


            /*
             * ditch illegal values now 
             */
            if (set_value == RS_ACTIVE || set_value == RS_NOTINSERVICE) {
                return SNMP_ERR_INCONSISTENTVALUE;
            }
        } else {
            /*
             * row exists.  Check for a valid state change 
             */
            if (set_value == RS_CREATEANDGO
                || set_value == RS_CREATEANDWAIT) {
                /*
                 * can't create a row that exists 
                 */
                return SNMP_ERR_INCONSISTENTVALUE;
            }
            if ((set_value == RS_ACTIVE || set_value == RS_NOTINSERVICE) &&
                StorageTmp->snmpNotifyFilterProfileNameLen == 0) {
                /*
                 * can't activate row without a profile name
                 */
                return SNMP_ERR_INCONSISTENTVALUE;
            }
            /*
             * XXX: interaction with row storage type needed 
             */
        }

        /*
         * memory reseveration, final preparation... 
         */
        if (StorageTmp == NULL &&
            (set_value == RS_CREATEANDGO
             || set_value == RS_CREATEANDWAIT)) {
            /*
             * creation 
             */
            vars = NULL;

            snmp_varlist_add_variable(&vars, NULL, 0,
                                      ASN_PRIV_IMPLIED_OCTET_STR, NULL, 0);

            if (header_complex_parse_oid
                (&
                 (name
                  [sizeof(snmpNotifyFilterProfileTable_variables_oid) /
                   sizeof(oid) + 2]), newlen, vars) != SNMPERR_SUCCESS) {
                snmp_free_var(vars);
                return SNMP_ERR_INCONSISTENTNAME;
            }

            StorageNew =
                SNMP_MALLOC_STRUCT(snmpNotifyFilterProfileTable_data);
            if (StorageNew == NULL)
                return SNMP_ERR_GENERR;
            StorageNew->snmpTargetParamsName =
                netsnmp_memdup(vars->val.string, vars->val_len);
            StorageNew->snmpTargetParamsNameLen = vars->val_len;
            StorageNew->snmpNotifyFilterProfileStorType = ST_NONVOLATILE;

            StorageNew->snmpNotifyFilterProfileRowStatus = RS_NOTREADY;
            snmp_free_var(vars);
        }


        break;

    case RESERVE2:
        break;

    case FREE:
        /*
         * XXX: free, zero vars 
         */
        /*
         * Release any resources that have been allocated 
         */
        if (StorageNew != NULL) {
            snmpNotifyFilterProfileTable_free(StorageNew);
            StorageNew = NULL;
        }
        break;




    case ACTION:
        /*
         * The variable has been stored in set_value for you to
         * use, and you have just been asked to do something with
         * it.  Note that anything done here must be reversable in
         * the UNDO case 
         */

        if (StorageTmp == NULL &&
            (set_value == RS_CREATEANDGO ||
             set_value == RS_CREATEANDWAIT)) {
            /*
             * row creation, so add it 
             */
            if (StorageNew != NULL)
                snmpNotifyFilterProfileTable_add(StorageNew);
            /*
             * XXX: ack, and if it is NULL? 
             */
        } else if (set_value != RS_DESTROY) {
            /*
             * set the flag? 
             */
            if (StorageTmp == NULL)
                return SNMP_ERR_GENERR; /* should never ever get here */
            
            old_value = StorageTmp->snmpNotifyFilterProfileRowStatus;
            StorageTmp->snmpNotifyFilterProfileRowStatus =
                *((long *) var_val);
        } else {
            /*
             * destroy...  extract it for now 
             */
            if (StorageTmp) {
                StorageDel =
                    snmpNotifyFilterProfileTable_extract(StorageTmp);
            }

        }
        break;

    case UNDO:
        /*
         * Back out any changes made in the ACTION case 
         */
        if (StorageTmp == NULL &&
            (set_value == RS_CREATEANDGO ||
             set_value == RS_CREATEANDWAIT)) {
            /*
             * row creation, so remove it again 
             */
            StorageDel =
                snmpNotifyFilterProfileTable_extract(StorageNew);
            /*
             * XXX: free it 
             */
        } else if (StorageDel != NULL) {
            /*
             * row deletion, so add it again 
             */
            snmpNotifyFilterProfileTable_add(StorageDel);
            StorageDel = NULL;
        } else if (set_value != RS_DESTROY) {
            if (StorageTmp)
                StorageTmp->snmpNotifyFilterProfileRowStatus = old_value;
        }
        break;




    case COMMIT:
        /*
         * Things are working well, so it's now safe to make the change
         * permanently.  Make sure that anything done here can't fail! 
         */
        if (StorageDel != NULL) {
            snmpNotifyFilterProfileTable_free(StorageDel);
            StorageDel = NULL;
        }
        if (StorageTmp && set_value == RS_CREATEANDGO) {
            if (StorageTmp->snmpNotifyFilterProfileNameLen)
                StorageTmp->snmpNotifyFilterProfileRowStatus = RS_ACTIVE;
            StorageNew = NULL;
        } else if (StorageTmp && set_value == RS_CREATEANDWAIT) {
            if (StorageTmp->snmpNotifyFilterProfileNameLen)
                StorageTmp->snmpNotifyFilterProfileRowStatus = RS_NOTINSERVICE;
            StorageNew = NULL;
        }
        snmp_store_needed(NULL);
        break;
    }
    return SNMP_ERR_NOERROR;
}


#endif /* !NETSNMP_NO_WRITE_SUPPORT */ 
