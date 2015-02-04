/*
 * Copyright (c) 2014, AirBitz, Inc.
 * All rights reserved.
 *
 * See the LICENSE file for more information.
 */

#include "Commands.hpp"
#include "../src/LoginShim.hpp"
#include "../abcd/Account.hpp"
#include "../abcd/Bridge.hpp"
#include "../abcd/Exchanges.hpp"
#include "../abcd/Wallet.hpp"
#include "../abcd/util/Crypto.hpp"
#include "../abcd/util/FileIO.hpp"
#include "../abcd/util/Util.hpp"
#include <wallet/wallet.hpp>
#include <iostream>

using namespace abcd;

/**
 * Reads a file into memory.
 */
static char *Slurp(const char *szFilename)
{
    // Where's the error checking?
    FILE *f;
    long l;
    char *buffer;

    f = fopen(szFilename, "r");
    fseek(f , 0L , SEEK_END);
    l = ftell(f);
    rewind(f);

    buffer = (char *)malloc(l + 1);
    if (!buffer) {
        return NULL;
    }
    if (fread(buffer, l, 1, f) != 1) {
        return NULL;
    }

    fclose(f);
    return buffer;
}

Status accountDecrypt(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... account-decrypt <user> <pass> <filename>\n"
            "note: The filename is account-relative.");

    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));

    std::string file = pKeys->szSyncDir;
    file += "/";
    file += argv[2];

    AutoU08Buf data;
    ABC_CHECK_OLD(ABC_CryptoDecryptJSONFile(file.c_str(), pKeys->MK, &data, &error));
    fwrite(data.p, data.end - data.p, 1, stdout);
    printf("\n");

    return Status();
}

Status accountEncrypt(int argc, char *argv[])
{

    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... account-encrypt <user> <pass> <filename>\n"
            "note: The filename is account-relative.");

    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));

    std::string file = pKeys->szSyncDir;
    file += "/";
    file += argv[2];

    AutoString szContents = Slurp(file.c_str());
    AutoString szEncrypted;
    if (szContents)
    {
        tABC_U08Buf data; // Do not free
        ABC_BUF_SET_PTR(data, (unsigned char *) szContents.get(), strlen(szContents));

        ABC_CHECK_OLD(ABC_CryptoEncryptJSONString(data, pKeys->MK,
            ABC_CryptoType_AES256, &szEncrypted.get(), &error));
        printf("%s\n", szEncrypted.get());
    }

    return Status();
}

Status addCategory(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... add-category <user> <pass> <category>");

    ABC_CHECK_OLD(ABC_AddCategory(argv[0], argv[1], argv[2], &error));

    return Status();
}

Status changePassword(int argc, char *argv[])
{
    if (argc != 4)
        return ABC_ERROR(ABC_CC_Error, "usage: ... change-password <pw|ra> <user> <pass|ra> <new-pass>");

    if (strncmp(argv[0], "pw", 2) == 0)
        ABC_CHECK_OLD(ABC_ChangePassword(argv[1], argv[2], argv[3], NULL, NULL, NULL, &error));
    else
        ABC_CHECK_OLD(ABC_ChangePasswordWithRecoveryAnswers(argv[1], argv[2], argv[3], NULL, NULL, NULL, &error));

    return Status();
}

Status checkPassword(int argc, char *argv[])
{
    if (argc != 1)
        return ABC_ERROR(ABC_CC_Error, "usage: ... check-password <pass>");

    double secondsToCrack;
    unsigned int count = 0;
    tABC_PasswordRule **aRules = NULL;
    ABC_CHECK_OLD(ABC_CheckPassword(argv[0], &secondsToCrack, &aRules, &count, &error));

    for (unsigned i = 0; i < count; ++i)
    {
        printf("%s: %d\n", aRules[i]->szDescription, aRules[i]->bPassed);
    }
    printf("Time to Crack: %f\n", secondsToCrack);
    ABC_FreePasswordRuleArray(aRules, count);

    return Status();
}

Status checkRecoveryAnswers(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... check-recovery-answers <user> <ras>");

    AutoString szQuestions;
    ABC_CHECK_OLD(ABC_GetRecoveryQuestions(argv[0], &szQuestions.get(), &error));
    printf("%s\n", szQuestions.get());

    bool bValid = false;
    ABC_CHECK_OLD(ABC_CheckRecoveryAnswers(argv[0], argv[1], &bValid, &error));
    printf("%s\n", bValid ? "Valid!" : "Invalid!");

    return Status();
}

Status createAccount(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... create-account <user> <pass>");

    ABC_CHECK_OLD(ABC_CreateAccount(argv[0], argv[1], "1234", NULL, NULL, &error));

    return Status();
}

Status createWallet(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... create-wallet <user> <pass> <wallet-name>");

    tABC_RequestResults results;
    ABC_CHECK_OLD(ABC_CreateWallet(argv[0], argv[1], argv[2],
        CURRENCY_NUM_USD, 0, NULL, &results, &error));

    return Status();
}

Status dataSync(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... data-sync <user> <pass>");

    ABC_CHECK_OLD(ABC_SignIn(argv[0], argv[1], NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_DataSyncAll(argv[0], argv[1], NULL, NULL, &error));

    return Status();
}

Status generateAddresses(int argc, char *argv[])
{
    if (argc != 4)
        return ABC_ERROR(ABC_CC_Error, "usage: ... generate-addresses <user> <pass> <wallet-name> <count>");

    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));

    tABC_U08Buf data; // Do not free
    ABC_CHECK_OLD(ABC_WalletGetBitcoinPrivateSeed(ABC_WalletID(pKeys, argv[2]), &data, &error));

    libbitcoin::data_chunk seed(data.p, data.end);
    libwallet::hd_private_key m(seed);
    libwallet::hd_private_key m0 = m.generate_private_key(0);
    libwallet::hd_private_key m00 = m0.generate_private_key(0);
    long max = strtol(argv[3], 0, 10);
    for (int i = 0; i < max; ++i)
    {
        libwallet::hd_private_key m00n = m00.generate_private_key(i);
        std::cout << "watch " << m00n.address().encoded() << std::endl;
    }

    return Status();
}

Status getBitcoinSeed(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... get-bitcoin-seed <user> <pass> <wallet-name>");

    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));

    tABC_U08Buf data; // Do not free
    ABC_CHECK_OLD(ABC_WalletGetBitcoinPrivateSeed(ABC_WalletID(pKeys, argv[2]), &data, &error));

    AutoString szSeed;
    ABC_CHECK_OLD(ABC_CryptoHexEncode(data, &szSeed.get(), &error));
    printf("%s\n", szSeed.get());

    return Status();
}

Status getCategories(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... get-categories <user> <pass>");

    AutoStringArray categories;
    ABC_CHECK_OLD(ABC_GetCategories(argv[0], argv[1], &categories.data, &categories.size, &error));

    printf("Categories:\n");
    for (unsigned i = 0; i < categories.size; ++i)
    {
        printf("\t%s\n", categories.data[i]);
    }

    return Status();
}

Status getExchangeRate(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... get-exchange-rate <user> <pass>");

    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_USD, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_AUD, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_CAD, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_CNY, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_CUP, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_HKD, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_MXN, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_NZD, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_PHP, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_GBP, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_USD, NULL, NULL, &error));
    ABC_CHECK_OLD(ABC_RequestExchangeRateUpdate(argv[0], argv[1], CURRENCY_NUM_EUR, NULL, NULL, &error));

    return Status();
}

Status getQuestionChoices(int argc, char *argv[])
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: ... get-question-choices");

    AutoFree<tABC_QuestionChoices, ABC_FreeQuestionChoices> pChoices;
    ABC_CHECK_OLD(ABC_GetQuestionChoices(&pChoices.get(), &error));

    printf("Choices:\n");
    for (unsigned i = 0; i < pChoices->numChoices; ++i)
    {
        printf(" %s (%s, %d)\n", pChoices->aChoices[i]->szQuestion,
                                  pChoices->aChoices[i]->szCategory,
                                  pChoices->aChoices[i]->minAnswerLength);
    }

    return Status();
}

Status getSettings(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... get-settings <user> <pass>");

    AutoFree<tABC_AccountSettings, ABC_FreeAccountSettings> pSettings;
    ABC_CHECK_OLD(ABC_LoadAccountSettings(argv[0], argv[1], &pSettings.get(), &error));

    printf("First name: %s\n", pSettings->szFirstName ? pSettings->szFirstName : "(none)");
    printf("Last name: %s\n", pSettings->szLastName ? pSettings->szLastName : "(none)");
    printf("Nickname: %s\n", pSettings->szNickname ? pSettings->szNickname : "(none)");
    printf("PIN: %s\n", pSettings->szPIN ? pSettings->szPIN : "(none)");
    printf("List name on payments: %s\n", pSettings->bNameOnPayments ? "yes" : "no");
    printf("Minutes before auto logout: %d\n", pSettings->minutesAutoLogout);
    printf("Language: %s\n", pSettings->szLanguage);
    printf("Currency num: %d\n", pSettings->currencyNum);
    printf("Advanced features: %s\n", pSettings->bAdvancedFeatures ? "yes" : "no");
    printf("Denomination satoshi: %ld\n", pSettings->bitcoinDenomination.satoshi);
    printf("Denomination id: %d\n", pSettings->bitcoinDenomination.denominationType);
    printf("TwoFactor: %d\n", pSettings->bTwoFactorEnabled);
    printf("Daily Spend Enabled: %d\n", pSettings->bDailySpendLimit);
    printf("Daily Spend Limit: %ld\n", (long) pSettings->dailySpendLimitSatoshis);
    printf("PIN Spend Enabled: %d\n", pSettings->bSpendRequirePin);
    printf("PIN Spend Limit: %ld\n", (long) pSettings->spendRequirePinSatoshis);
    printf("Exchange rate sources:\n");
    for (unsigned i = 0; i < pSettings->exchangeRateSources.numSources; i++)
    {
        printf("\tcurrency: %d\tsource: %s\n",
            pSettings->exchangeRateSources.aSources[i]->currencyNum,
            pSettings->exchangeRateSources.aSources[i]->szSource);
    }

    return Status();
}

Status getWalletInfo(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... get-wallet-info <user> <pass> <wallet-name>");

    // TODO: This no longer works without a running watcher!
    AutoFree<tABC_WalletInfo, ABC_WalletFreeInfo> pInfo;
    ABC_CHECK_OLD(ABC_GetWalletInfo(argv[0], argv[1], argv[2], &pInfo.get(), &error));

    return Status();
}

Status listAccounts(int argc, char *argv[])
{
    if (argc != 0)
        return ABC_ERROR(ABC_CC_Error, "usage: ... list-accounts");

    AutoString usernames;
    ABC_CHECK_OLD(ABC_ListAccounts(&usernames.get(), &error));
    printf("Usernames:\n%s", usernames.get());

    return Status();
}

Status listWallets(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... list-wallets <user> <pass>");

    // Setup:
    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));
    ABC_CHECK_OLD(ABC_DataSyncAll(argv[0], argv[1], NULL, NULL, &error));

    // Iterate over wallets:
    AutoStringArray uuids;
    ABC_CHECK_OLD(ABC_GetWalletUUIDs(argv[0], argv[1],
        &uuids.data, &uuids.size, &error));
    for (unsigned i = 0; i < uuids.size; ++i)
    {
        tABC_Error error;

        // Print the UUID:
        printf("%s: ", uuids.data[i]);

        // Get wallet name filename:
        char *szDir;
        char szFilename[ABC_FILEIO_MAX_PATH_LENGTH];
        ABC_CHECK_OLD(ABC_WalletGetDirName(&szDir, uuids.data[i], &error));
        snprintf(szFilename, sizeof(szFilename),
            "%s/sync/WalletName.json", szDir);

        // Print wallet name:
        AutoU08Buf data;
        AutoAccountWalletInfo info;
        ABC_CHECK_OLD(ABC_AccountWalletLoad(pKeys, uuids.data[i], &info, &error));
        if (ABC_CC_Ok == ABC_CryptoDecryptJSONFile(szFilename, info.MK, &data, &error))
        {
            fwrite(data.p, data.end - data.p, 1, stdout);
            printf("\n");
        }
    }
    printf("\n");

    return Status();
}

Status pinLogin(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... pin-login <user> <pin>");

    bool bExists;
    ABC_CHECK_OLD(ABC_PinLoginExists(argv[0], &bExists, &error));
    if (bExists)
    {
        ABC_CHECK_OLD(ABC_PinLogin(argv[0], argv[1], &error));
    }
    else
    {
        printf("Login expired\n");
    }

    return Status();
}


Status pinLoginSetup(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... pin-login-setup <user> <pass>");

    ABC_CHECK_OLD(ABC_PinSetup(argv[0], argv[1], &error));

    return Status();
}

Status recoveryReminderSet(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... recovery-reminder-set <user> <pass> <n>");

    AutoFree<tABC_AccountSettings, ABC_FreeAccountSettings> pSettings;
    ABC_CHECK_OLD(ABC_LoadAccountSettings(argv[0], argv[1], &pSettings.get(), &error));
    printf("Old Reminder Count: %d\n", pSettings->recoveryReminderCount);

    pSettings->recoveryReminderCount = strtol(argv[2], 0, 10);
    ABC_CHECK_OLD(ABC_UpdateAccountSettings(argv[0], argv[1], pSettings, &error));

    return Status();
}

Status removeCategory(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... remove-category <user> <pass> <category>");

    ABC_CHECK_OLD(ABC_RemoveCategory(argv[0], argv[1], argv[2], &error));

    return Status();
}

Status searchBitcoinSeed(int argc, char *argv[])
{
    if (argc != 6)
        return ABC_ERROR(ABC_CC_Error, "usage: ... search-bitcoin-seed <user> <pass> <wallet-name> <addr> <start> <end>");

    long start = strtol(argv[4], 0, 10);
    long end = strtol(argv[5], 0, 10);
    char *szMatchAddr = argv[3];

    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));

    tABC_U08Buf data; // Do not free
    ABC_CHECK_OLD(ABC_WalletGetBitcoinPrivateSeed(ABC_WalletID(pKeys, argv[2]), &data, &error));

    for (long i = start, c = 0; i <= end; i++, ++c)
    {
        AutoString szPubAddress;
        ABC_BridgeGetBitcoinPubAddress(&szPubAddress.get(), data, (int32_t) i, NULL);
        if (strncmp(szPubAddress.get(), szMatchAddr, strlen(szMatchAddr)) == 0)
        {
            printf("Found %s at %ld\n", szMatchAddr, i);
            break;
        }
        if (c == 100000)
        {
            printf("%ld\n", i);
            c = 0;
        }
    }

    return Status();
}

Status setNickname(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... set-nickname <user> <pass> <name>");

    AutoFree<tABC_AccountSettings, ABC_FreeAccountSettings> pSettings;
    ABC_CHECK_OLD(ABC_LoadAccountSettings(argv[0], argv[1], &pSettings.get(), &error));
    free(pSettings->szNickname);
    pSettings->szNickname = strdup(argv[2]);
    ABC_CHECK_OLD(ABC_UpdateAccountSettings(argv[0], argv[1], pSettings, &error));

    return Status();
}

Status signIn(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... sign-in <user> <pass>");

    ABC_CHECK_OLD(ABC_SignIn(argv[0], argv[1], NULL, NULL, &error));

    return Status();
}

Status otpOn(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-on <user> <pass>");

    ABC_CHECK_OLD(ABC_EnableTwoFactor(argv[0], argv[1], &error));

    return Status();
}

Status otpOff(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-off <user> <pass>");

    ABC_CHECK_OLD(ABC_DisableTwoFactor(argv[0], argv[1], &error));

    return Status();
}

Status otpStatus(int argc, char *argv[])
{
    bool on = false;;
    long timeout = 0;
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-status <user> <pass>");

    ABC_CHECK_OLD(ABC_StatusTwoFactor(argv[0], argv[1], &on, &timeout, &error));

    std::cout << "Otp On: " << on << std::endl;
    std::cout << "Otp Timeout: " << timeout << std::endl;

    return Status();
}

Status otpGetSecret(int argc, char *argv[])
{
    char *szSecret = NULL;
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-get-secret <user> <pass>");

    ABC_CHECK_OLD(ABC_GetTwoFactorSecret(argv[0], argv[1], &szSecret, &error));

    std::cout << "Secret: " << szSecret << std::endl;

    return Status();
}

Status otpSetSecret(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-get-secret <user> <pass> <new-secret>");

    ABC_CHECK_OLD(ABC_SetTwoFactorSecret(argv[0], argv[1], argv[2], true, &error));

    return Status();
}

Status otpSignIn(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-get-secret <user> <pass> <secret>");

    ABC_CHECK_OLD(ABC_TwoFactorSignIn(argv[0], argv[1], argv[2], &error));

    return Status();
}

abcd::Status otpRequestReset(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-get-secret <user> <pass> <secret>");

    ABC_CHECK_OLD(ABC_RequestTwoFactorReset(argv[0], argv[1], &error));

    return Status();
}

abcd::Status otpRequestPending(int argc, char *argv[])
{
    bool *pending = NULL;
    int i = 0;
    if (argc < 1)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-reset-pending <user-1> <user-2> ...<user-n>");

    ABC_CHECK_OLD(ABC_IsTwoFactorResetPending((const char **)argv, argc, &pending, &error));
    std::cout << "Pending?" << std::endl;
    for (i = 0; i < argc; ++i)
    {
        std::cout << argv[i] << ": ";
        if (pending[i])
            std::cout << "Yes. " << std::endl;
        else
            std::cout << "No. " << std::endl;
    }
    if (pending)
        free(pending);
    return Status();
}

abcd::Status otpCancelReset(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-cancel-pending <user> <pass>");

    ABC_CHECK_OLD(ABC_CancelTwoFactorReset(argv[0], argv[1], &error));

    return Status();
}

abcd::Status otpQrCode(int argc, char *argv[])
{
    unsigned char *aData;
    unsigned int width;
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... otp-qr-code <user> <pass>");

    ABC_CHECK_OLD(ABC_GetTwoFactorQrCode(argv[0], argv[1], &aData, &width, &error));
    if (aData)
        free(aData);

    return Status();
}

Status uploadLogs(int argc, char *argv[])
{
    if (argc != 2)
        return ABC_ERROR(ABC_CC_Error, "usage: ... upload-logs <user> <pass>");

    // TODO: Command non-functional without a watcher thread!
    ABC_CHECK_OLD(ABC_UploadLogs(argv[0], argv[1], &error));

    return Status();
}

Status walletDecrypt(int argc, char *argv[])
{
    if (argc != 4)
        return ABC_ERROR(ABC_CC_Error, "usage: ... wallet-decrypt <user> <pass> <wallet-name> <file>");

    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));

    AutoAccountWalletInfo info;
    ABC_CHECK_OLD(ABC_AccountWalletLoad(pKeys, argv[2], &info, &error));

    AutoU08Buf data;
    ABC_CHECK_OLD(ABC_CryptoDecryptJSONFile(argv[3], info.MK, &data, &error));
    fwrite(data.p, data.end - data.p, 1, stdout);
    printf("\n");

    return Status();
}

Status walletEncrypt(int argc, char *argv[])
{
    if (argc != 4)
        return ABC_ERROR(ABC_CC_Error, "usage: ... wallet-encrypt <user> <pass> <wallet-name> <file>");

    AutoSyncKeys pKeys;
    ABC_CHECK_OLD(ABC_LoginShimGetSyncKeys(argv[0], argv[1], &pKeys.get(), &error));

    AutoAccountWalletInfo info;
    ABC_CHECK_OLD(ABC_AccountWalletLoad(pKeys, argv[2], &info, &error));

    AutoString szContents = Slurp(argv[3]);
    if (szContents)
    {
        tABC_U08Buf data; // Do not free
        ABC_BUF_SET_PTR(data, (unsigned char *) szContents.get(), strlen(szContents));

        AutoString szEncrypted;
        ABC_CHECK_OLD(ABC_CryptoEncryptJSONString(data, info.MK,
            ABC_CryptoType_AES256, &szEncrypted.get(), &error));
        printf("%s\n", szEncrypted.get());
    }

    return Status();
}

Status walletGetAddress(int argc, char *argv[])
{
    if (argc != 3)
        return ABC_ERROR(ABC_CC_Error, "usage: ... wallet-get-address <user> <pass> <wallet-name>");

    tABC_TxDetails details;
    details.szName = const_cast<char*>("");
    details.szCategory = const_cast<char*>("");
    details.szNotes = const_cast<char*>("");
    details.attributes = 0x0;
    details.bizId = 0x0;
    details.attributes = 0x0;
    details.bizId = 0;
    details.amountSatoshi = 0;
    details.amountCurrency = 0;
    details.amountFeesAirbitzSatoshi = 0;
    details.amountFeesMinersSatoshi = 0;

    AutoString szRequestID;
    AutoString szAddress;
    AutoString szURI;
    unsigned char *szData = NULL;
    unsigned int width = 0;
    printf("starting...");
    ABC_CHECK_OLD(ABC_CreateReceiveRequest(argv[0], argv[1], argv[2],
        &details, &szRequestID.get(), &error));

    ABC_CHECK_OLD(ABC_GenerateRequestQRCode(argv[0], argv[1], argv[2],
        szRequestID, &szURI.get(), &szData, &width, &error));

    ABC_CHECK_OLD(ABC_GetRequestAddress(argv[0], argv[1], argv[2],
        szRequestID, &szAddress.get(), &error));

    printf("URI: %s\n", szURI.get());
    printf("Address: %s\n", szAddress.get());

    return Status();
}
