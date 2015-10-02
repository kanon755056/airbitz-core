/*
 *  Copyright (c) 2014, AirBitz, Inc.
 *  All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#ifndef ABCD_WALLET_ADDRESS_HPP
#define ABCD_WALLET_ADDRESS_HPP

#include "../util/Status.hpp"

namespace abcd {

class Wallet;

tABC_CC ABC_TxWatchAddresses(Wallet &self,
                             tABC_Error *pError);

tABC_CC ABC_TxTrashAddresses(Wallet &self,
                             tABC_TxDetails **ppDetails,
                             tABC_TxOutput **paAddresses,
                             unsigned int addressCount,
                             tABC_Error *pError);

tABC_CC ABC_TxCreateReceiveRequest(Wallet &self,
                                   tABC_TxDetails *pDetails,
                                   char **pszRequestID,
                                   bool bTransfer,
                                   tABC_Error *pError);

tABC_CC ABC_TxModifyReceiveRequest(Wallet &self,
                                   const char *szRequestID,
                                   tABC_TxDetails *pDetails,
                                   tABC_Error *pError);

tABC_CC ABC_TxFinalizeReceiveRequest(Wallet &self,
                                     const char *szRequestID,
                                     tABC_Error *pError);

tABC_CC ABC_TxCancelReceiveRequest(Wallet &self,
                                   const char *szRequestID,
                                   tABC_Error *pError);

tABC_CC ABC_TxGenerateRequestQRCode(Wallet &self,
                                    const char *szRequestID,
                                    char **pszURI,
                                    unsigned char **paData,
                                    unsigned int *pWidth,
                                    tABC_Error *pError);

tABC_CC ABC_TxGetRequestAddress(Wallet &self,
                                const char *szRequestID,
                                char **pszAddress,
                                tABC_Error *pError);

} // namespace abcd

#endif