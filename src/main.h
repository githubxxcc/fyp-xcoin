/*
 * =====================================================================================
 *
 *       Filename:  main.h
 *
 *    Description:  Adopted from original bitcoin
 *
 *        Version:  1.0
 *        Created:  07/09/18 17:27:03
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ricky (), 
 *   Organization:  
 *
 * =====================================================================================
 */

class COutPoint;
class CInPoint;
//class CDiskTxPos;
//class CCoinBase;
class CTxIn;
class CTxOut;
class CTransaction;
class CBlock;
class CBlockIndex;
//class CWalletTx;
//class CKeyItem;

//extern map<uint256, CTransaction> mapTransactions;
////extern map<uint256, CWalletTx> mapWallet;
////extern vector<pair<uint256, bool> > vWalletUpdated;
////extern CCriticalSection cs_mapWallet;
////extern map<vector<unsigned char>, CPrivKey> mapKeys;
////extern map<uint160, vector<unsigned char> > mapPubKeys;
////extern CCriticalSection cs_mapKeys;
////extern CKey keyUser;
//
////extern CCriticalSection cs_main;
//extern map<uint256, CBlockIndex*> mapBlockIndex;
//extern const uint256 hashGenesisBlock;
//extern CBlockIndex* pindexGenesisBlock;
//extern int nBestHeight;
//extern uint256 hashBestChain;
//extern CBlockIndex* pindexBest;
//extern unsigned int nTransactionsUpdated;
//extern string strSetDataDir;
////extern int nDropMessagesTest;
//
//// Settings
////extern int fGenerateBitcoins;
////extern int64 nTransactionFee;
////extern CAddress addrIncoming;


class COutPoint 
{
    uint256 hash;
    unsigned int n; /*  */
}


class CInPoint
{
    CTransaction* ptx;
    unsigned int n;  /* Index */
}


class CTxIn
{
    COutPoint prevout;
    unsigned int nSequence;
}

class CTxOut 
{
    int64 nValue; /*  Credit */
    //script no longer needed for indicating who to pay for
    int64 address; /*  Target address */
}

class CTransaction
{
    int nVersion;
    vector<CTxIn> vin;
    vector<CTxOut> vout;
    int nLockTime;

}


class CBlock
{
    //Header info embedded
    int nVersion;
    uint256 hashPrevBlock;
    unsigned int nTime;
    unsigned int nBits;
    unsigned int nNonce;
    
    vector<CTransaction> vtx;

}



class CBlockIndex 
{
    const uint256* phashBlock;
    CBlockIndex* pprev;
    CBlockIndex* pnext;
    /* How to locate the underlying block */
    //unsigned int nFile;
    //unsigned int nBlockPos;
    int nHeight;
    
    //From Block Header
    int nVersion;
    unsigned int nTimes;
    unsigned int nBits;
    unsigned int nNonce;
}



