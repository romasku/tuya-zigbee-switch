/*
 * reporting.c
 * Created: pvvx
 */
#include "reporting.h"
#include "utility.h"

// extern void reportAttr(reportCfgInfo_t *pEntry);

/*********************************************************************
 * @fn      reportAttr
 *
 * @brief
 *
 * @param   pEntry
 *
 * @return      NULL
 */
_CODE_ZCL_ void reportAttr(reportCfgInfo_t *pEntry)
{
        if(!zb_bindingTblSearched(pEntry->clusterID, pEntry->endPoint)){
                return;
        }

        epInfo_t dstEpInfo;
        TL_SETSTRUCTCONTENT(dstEpInfo, 0);

        dstEpInfo.dstAddrMode = APS_DSTADDR_EP_NOTPRESETNT;
        dstEpInfo.profileId = pEntry->profileID;

        zclAttrInfo_t *pAttrEntry = zcl_findAttribute(pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
        if(!pAttrEntry){
                //should not happen.
                ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
                return;
        }

        u16 len = zcl_getAttrSize(pAttrEntry->type, pAttrEntry->data);

        len = (len>8) ? (8):(len);

        //store for next compare
        memcpy(pEntry->prevData, pAttrEntry->data, len);

        zcl_sendReportCmd(pEntry->endPoint, &dstEpInfo,  TRUE, ZCL_FRAME_SERVER_CLIENT_DIR,
                                          pEntry->clusterID, pAttrEntry->id, pAttrEntry->type, pAttrEntry->data);
}



/*********************************************************************
 * @fn      app_chk_report
 *
 * @brief	check if there is report.
 *
 * @param   time from old check in sec
 *
 * @return	NULL
 */
void app_chk_report(u16 uptime_sec)
{
  zclAttrInfo_t *pAttrEntry = NULL;
  u16            len;
  bool           flg_report, flg_chk_attr;

  if (reportingTab.reportNum)
  {
    for (u8 i = 0; i < ZCL_REPORTING_TABLE_NUM; i++)
    {
      reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
      if (pEntry->used && (pEntry->maxInterval != 0xFFFF))
      {
        // used
        flg_chk_attr = false;
        flg_report   = false;

        // timer minInterval seconds
        if (pEntry->minIntCnt > uptime_sec)
        {
          pEntry->minIntCnt -= uptime_sec;
        }
        else
        {
          pEntry->minIntCnt = 0;
        }
        // timer maxInterval seconds
        if (pEntry->maxIntCnt > uptime_sec)
        {
          pEntry->maxIntCnt -= uptime_sec;
        }
        else
        {
          pEntry->maxIntCnt = 0;
        }

        if (pEntry->maxIntCnt == 0)
        {
          flg_report = true;
        }
        else if (pEntry->minIntCnt == 0)
        {
          flg_chk_attr = true;
        }
        if (flg_chk_attr || flg_report)
        {
          pAttrEntry = zcl_findAttribute(pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
          if (!pAttrEntry)
          {
            printf("Failed to get entry!!!\r\n");
            // should not happen.
            ZB_EXCEPTION_POST(SYS_EXCEPTTION_ZB_ZCL_ENTRY);
            return;
          }
        }
        if (flg_chk_attr)
        {
          // report pAttrEntry

          if (zcl_analogDataType(pAttrEntry->type))
          {
            if (reportableChangeValueChk(pAttrEntry->type, pAttrEntry->data, pEntry->prevData, pEntry->reportableChange))
            {
              flg_report = true;
            }
          }
          else
          {
            len = zcl_getAttrSize(pAttrEntry->type, pAttrEntry->data);
            len = (len > 8) ? (8): (len);
            if (memcmp(pEntry->prevData, pAttrEntry->data, len) != SUCCESS)
            {
              flg_report = true;
            }
          }
        }
        if (flg_report)
        {
          printf("Sending report for %d %d %d \r\n", pEntry->endPoint, pEntry->clusterID, pEntry->attrID);
          pEntry->minIntCnt = pEntry->minInterval;
          pEntry->maxIntCnt = pEntry->maxInterval;

          reportAttr(pEntry);
        }
      }
    }
  }
  else
  {
    // no report
  }
}
