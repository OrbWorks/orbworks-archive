/$ neterror.h
// neterror(int err) - returns a string representation of the error
// code returned by any of the network functions

neterror(int err) {
    string res = "Unknown network error";
    switch (err) {
        case 1: res = "netErrInvalidAddrString"; break;
        case 2: res = "netErrOther"; break;
        case 4612: res = "netErrParamErr"; break;
        case 4613: res = "netErrNoMoreSockets"; break;
        case 4614: res = "netErrOutOfResources"; break;
        case 4615: res = "netErrOutOfMemory"; break;
        case 4616: res = "netErrSocketNotOpen"; break;
        case 4617: res = "netErrSocketBusy"; break;
        case 4618: res = "netErrMessageTooBig"; break;
        case 4619: res = "netErrSocketNotConnected"; break;
        case 4620: res = "netErrNoInterfaces"; break;
        case 4623: res = "netErrPortInUse"; break;
        case 4624: res = "netErrQuietTimeNotElapsed"; break;
        case 4625: res = "netErrInternal"; break;
        case 4626: res = "netErrTimeout"; break;
        case 4627: res = "netErrSocketAlreadyConnected"; break;
        case 4628: res = "netErrSocketClosedByRemote"; break;
        case 4629: res = "netErrOutOfCmdBlocks"; break;
        case 4630: res = "netErrWrongSocketType"; break;
        case 4631: res = "netErrSocketNotListening"; break;
        case 4632: res = "netErrUnknownSetting"; break;
        case 4651: res = "netErrUnknownProtocol"; break;
        case 4653: res = "netErrUnreachableDest"; break;
        case 4655: res = "netErrWouldBlock"; break;
        case 4661: res = "netErrDNSNameTooLong"; break;
        case 4662: res = "netErrDNSBadName"; break;
        case 4663: res = "netErrDNSBadArgs"; break;
        case 4664: res = "netErrDNSLabelTooLong"; break;
        case 4665: res = "netErrDNSAllocationFailure"; break;
        case 4666: res = "netErrDNSTimeout"; break;
        case 4667: res = "netErrDNSUnreachable"; break;
        case 4668: res = "netErrDNSFormat"; break;
        case 4669: res = "netErrDNSServerFailure"; break;
        case 4670: res = "netErrDNSNonexistantName"; break;
        case 4671: res = "netErrDNSNIY"; break;
        case 4672: res = "netErrDNSRefused"; break;
        case 4673: res = "netErrDNSImpossible"; break;
        case 4674: res = "netErrDNSNoRRS"; break;
        case 4675: res = "netErrDNSAborted"; break;
        case 4676: res = "netErrDNSBadProtocol"; break;
        case 4677: res = "netErrDNSTruncated"; break;
        case 4678: res = "netErrDNSNoRecursion"; break;
        case 4679: res = "netErrDNSIrrelevant"; break;
        case 4680: res = "netErrDNSNotInLocalCache"; break;
        case 4681: res = "netErrDNSNoPort"; break;
        case 4682: res = "netErrIPCantFragment"; break;
        case 4683: res = "netErrIPNoRoute"; break;
        case 4684: res = "netErrIPNoSrc"; break;
        case 4685: res = "netErrIPNoDst"; break;
        case 4686: res = "netErrIPktOverflow"; break;
        case 4687: res = "netErrTooManyTCPConnections"; break;
        case 4688: res = "netErrNoDNSServers"; break;
        case 4689: res = "netErrInterfaceDown"; break;	
    }
    return res;
}
