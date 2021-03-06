#include "envswitch.h"
#include "resourcebroker/resourcebroker_API.h"
#include "resourcebroker/resourcebroker_NONE.h"
#include "resourcebroker/resourcebroker_LIBYARN.h"

bool					 ResourceManagerIsForked;
RB_FunctionEntriesData 	 CurrentRBImp;

void RB_prepareImplementation(enum RB_IMP_TYPE imptype)
{
	/* Initialize resource broker implement handlers. */
	CurrentRBImp.acquireResource    = NULL;
	CurrentRBImp.freeClusterReport  = NULL;
	CurrentRBImp.getClusterReport   = NULL;
	CurrentRBImp.handleNotification = NULL;
	CurrentRBImp.handleSigSIGCHLD   = NULL;
	CurrentRBImp.returnResource     = NULL;
	CurrentRBImp.start				= NULL;
	CurrentRBImp.stop				= NULL;

	switch(imptype) {
	case NONE_HAWQ2:
		RB_NONE_createEntries(&CurrentRBImp);
		break;
	case YARN_LIBYARN:
		RB_LIBYARN_createEntries(&CurrentRBImp);
		break;
	default:
		Assert(false);
	}
}

/* Start resource broker service. */
int RB_start(bool isforked)
{
	return CurrentRBImp.start != NULL ? CurrentRBImp.start(isforked) : FUNC_RETURN_OK;
}
/* Stop resource broker service. */
int RB_stop(void)
{
	return CurrentRBImp.stop != NULL ? CurrentRBImp.stop() : FUNC_RETURN_OK;
}

int RB_getClusterReport(const char *queuename, List **machines, double *maxcapacity)
{
	if ( CurrentRBImp.getClusterReport != NULL )
	{
		return CurrentRBImp.getClusterReport(queuename, machines, maxcapacity);
	}

	*maxcapacity = 1;
	return FUNC_RETURN_OK;
}

/* Acquire and return resource. */
int RB_acquireResource(uint32_t memorymb, uint32_t core, List *preferred)
{
	Assert(CurrentRBImp.acquireResource != NULL);
	return CurrentRBImp.acquireResource(memorymb, core, preferred);
}

int RB_returnResource(List **containers)
{
	Assert(CurrentRBImp.returnResource != NULL);
	return CurrentRBImp.returnResource(containers);
}

int RB_getContainerReport(List **ctnstat)
{
	if( CurrentRBImp.getContainerReport != NULL )
	{
		return CurrentRBImp.getContainerReport(ctnstat);
	}
	return FUNC_RETURN_OK;
}

int RB_handleNotifications(void)
{
	return CurrentRBImp.handleNotification != NULL ?
		   CurrentRBImp.handleNotification() :
		   FUNC_RETURN_OK;
}

void RB_freeClusterReport(List **segments)
{
	if (CurrentRBImp.freeClusterReport != NULL)
	{
		CurrentRBImp.freeClusterReport(segments);
		return;
	}

	MEMORY_CONTEXT_SWITCH_TO(PCONTEXT)
	/* default implementation for freeing the cluster report objects. */
	while( list_length(*segments) > 0 )
	{
		SegInfo seginfo = (SegInfo)lfirst(list_head(*segments));
		*segments = list_delete_first(*segments);
		rm_pfree(PCONTEXT, seginfo);
	}
	MEMORY_CONTEXT_SWITCH_BACK
}

void RB_handleSignalSIGCHLD(void)
{
	if (CurrentRBImp.handleSigSIGCHLD != NULL)
	{
		CurrentRBImp.handleSigSIGCHLD();
	}
}

void RB_handleError(int errorcode)
{
	Assert(CurrentRBImp.handleError != NULL);
	CurrentRBImp.handleError(errorcode);
}
bool isCleanGRMResourceStatus(void)
{
	Assert(DRMGlobalInstance != NULL);
	return DRMGlobalInstance->ResBrokerTriggerCleanup == true;
}

void setCleanGRMResourceStatus(void)
{
	Assert(DRMGlobalInstance != NULL);
	DRMGlobalInstance->ResBrokerTriggerCleanup = true;
	elog(WARNING, "Resource manager went into cleanup phase due to error from "
				  "resource broker.");
}

void unsetCleanGRMResourceStatus(void)
{
	Assert(DRMGlobalInstance != NULL);
	DRMGlobalInstance->ResBrokerTriggerCleanup = false;
	elog(WARNING, "Resource manager left cleanup phase.");
}

void RB_clearResource(List **ctnl)
{
	while( (*ctnl) != NULL )
	{
		GRMContainer ctn = (GRMContainer)lfirst(list_head(*ctnl));
		MEMORY_CONTEXT_SWITCH_TO(PCONTEXT)
		(*ctnl) = list_delete_first(*ctnl);
		MEMORY_CONTEXT_SWITCH_BACK

		elog(LOG, "Resource broker dropped GRM container %d "
				  "(%d MB, %d CORE) on host %s",
				  ctn->ID,
				  ctn->MemoryMB,
				  ctn->Core,
				  ctn->HostName == NULL ? "NULL" : ctn->HostName);

		if ( ctn->CalcDecPending )
		{
			minusResourceBundleData(&(ctn->Resource->DecPending), ctn->MemoryMB, ctn->Core);
			Assert( ctn->Resource->DecPending.Core >= 0 );
			Assert( ctn->Resource->DecPending.MemoryMB >= 0 );
		}

		/* Destroy resource container. */
		freeGRMContainer(ctn);
		PRESPOOL->RetPendingContainerCount--;
	}
}
