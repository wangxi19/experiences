#ifndef EXPPARSER_HPP
#define EXPPARSER_HPP
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

using namespace std;
#ifndef unlikely
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#endif
#ifndef likely
#define likely(expr) __builtin_expect(!!(expr), 1)
#endif
class ExpParser {
    struct OpeNode
    {
        OpeNode() {}

        // !left
        bool UnLeft{false};
        std::string leftElement;
        // !right
        bool UnRight{false};
        std::string rightElement;
        std::string opeSign;
        bool brancheNode{false};
        std::vector<OpeNode*> branch;
        // The "C"
        OpeNode* CParentNext{nullptr};
        std::vector<OpeNode*> next;
        OpeNode* pre{nullptr};
    };

    enum opeType {
        opeType_None = 0,
        opeType_Element = 1,
        opeType_C = '(',
        opeType_i = '!',
        opeType_Reverse_C = ')',
        opeType_Group1 = 100,
        opeType_Group2 = 101,
        opeType_End = 200
    };

    enum opeTypePrecise {
        opeTypePrecise_None = 0,
        opeTypePrecise_Element = 1,
        opeTypePrecise_C,
        opeTypePrecise_i,
        opeTypePrecise_Reverse_C,
        opeTypePrecise_2_e,// ==
        opeTypePrecise_i_e,// !=
        opeTypePrecise_gt,// >
        opeTypePrecise_gt_e,// >=
        opeTypePrecise_lt,// <
        opeTypePrecise_lt_e,// <=,
        opeTypePrecise_or,// or
        opeTypePrecise_and// and
    };

    std::string operators[11] = {
        //follower must be item
        //Group 1
        "==",
        "!=",
        ">",
        ">=",
        "<",
        "<=",
        //follower must be item or (
        "!",
        //follower must be Group 1 or Group 2
        ")",
        //These below three signs's follower must be exp
        "(",
        //Group 2
        "or",
        "and"
    };
    //state machine
    std::string mExpression;
    size_t mCurExpIdx{0};
    std::string mCurVal;
    opeType mCurOpTyp{opeType_None};
    //   `mTolerance`: The number of ')' that  can be tolerated
    int mTolerance{0};
    //   `mG1TTL`: std::tuple<int, int>;
    //           The first int is the previous '(' number
    //           the second int is the TTL
    //   The TTL will be minused 1 only if the first int is equal to or greater than `mTolerance`
    //   The TTL minus 1 only after passed the function
    //   So the valid TTL in the current function is the original unminused value
    //    std::vector<std::tuple<int, int>> mG1TTL;
    std::vector<opeType> mOpTypTrail;
    std::vector<opeTypePrecise> mOpTypTrailPrecise;
    typedef bool(*PfnOperator) (bool Unleft, const std::string& left, const std::string& ope, bool UnRight, const std::string& right, void* ctxt);
    OpeNode* mRootNode{nullptr};
    OpeNode* mCurNode{nullptr};
    std::vector<OpeNode*> mCStack;
    std::vector<OpeNode*> mDeleteVec;
    PfnOperator mOpeCallBack{nullptr};
    std::string mErrMsg;

public:
    explicit ExpParser() {

    }
    ~ExpParser()
    {
        for (auto ptr: mDeleteVec)
            delete ptr;

        mDeleteVec.clear();
    }

    const std::string& getLastError()
    {
        return mErrMsg;
    }

    void setOpeCallBack(const PfnOperator &opeCallBack)
    {
        mOpeCallBack = opeCallBack;
    }

    void setExp(const std::string& exp, const PfnOperator &opeCallBack) {
        mExpression = exp;
        mCurExpIdx = 0;
        mCurVal.clear();
        mCurOpTyp = opeType_None;
        mTolerance = 0;
        //        mG1TTL.clear();
        mOpTypTrail.clear();
        mOpTypTrailPrecise.clear();

        mCStack.clear();
        for (auto ptr: mDeleteVec)
            delete ptr;

        mDeleteVec.clear();
        mRootNode = nullptr;
        mCurNode = nullptr;
        mOpeCallBack = opeCallBack;
    }

    bool startParsing() {
        bool ret{false};
        mErrMsg.clear();
        std::string curVal;
        mRootNode = getOpeNode();
        mRootNode->opeSign.assign("root");
        mCurNode = mRootNode;
        getToken();
        curVal = mCurVal;
        switch (mCurOpTyp) {
        case opeType_Element:
            ret = stateElement();
            break;
        case opeType_C:
            ret = stateC();
            break;
        case opeType_i:
            ret = stateRi();
            break;
        default:
            break;
        }

        if (!ret)
            //            cerr << "Invalid state after `" << curVal << "`" << endl;
            mErrMsg += " Invalid state after `" + curVal + "`\n";

        return ret;
    }

    inline bool runExp(void* param) {
        if (unlikely(!mOpeCallBack))
            return false;

        OpeNode* curNode = mRootNode->next.size() > 0 ? mRootNode->next[0] : nullptr;
        if (curNode == nullptr)
            return true;

        return runExpRecur(curNode, param);
    }

private:
    void getToken() {
        mCurVal.clear();
        auto expIdx = mCurExpIdx;
        auto lc = std::locale();
        size_t tmpIdx{0};
        for (size_t idx = expIdx; idx < mExpression.size(); idx++) {
            if (mExpression[idx] == ' ')
                continue;

            switch (mExpression[idx]) {
            case '=':
            case '!':
            case '>':
            case '<':
                mCurOpTyp = opeType_Group1;
                if (idx < mExpression.size() - 1) {
                    if (mExpression[idx] == '=') {
                        if (mExpression[idx+1] == '=') {
                            mCurExpIdx = idx + 2;
                            mCurVal.assign("==");
                        } else {
                            mCurExpIdx = idx + 1;
                            mCurVal.assign("=");
                        }
                    } else if (mExpression[idx] == '!') {
                        if (mExpression[idx+1] == '=') {
                            mCurExpIdx = idx + 2;
                            mCurVal.assign("!=");
                        } else {
                            mCurExpIdx = idx + 1;
                            mCurVal.assign("!");
                            mCurOpTyp = opeType_i;
                        }
                    } else if (mExpression[idx] == '>') {
                        if (mExpression[idx+1] == '=') {
                            mCurExpIdx = idx + 2;
                            mCurVal.assign(">=");
                        } else {
                            mCurExpIdx = idx + 1;
                            mCurVal.assign(">");
                        }
                    } else if (mExpression[idx] == '<') {
                        if (mExpression[idx+1] == '=') {
                            mCurExpIdx = idx + 2;
                            mCurVal.assign("<=");
                        } else {
                            mCurExpIdx = idx + 1;
                            mCurVal.assign("<");
                        }
                    }
                } else {
                    mCurExpIdx = idx + 1;
                    mCurVal.assign({mExpression[idx]});
                    if (mExpression[idx] == '!')
                        mCurOpTyp = opeType_i;
                }

                break;
            case '(':
                mCurExpIdx = idx + 1;
                mCurVal.assign("(");
                mCurOpTyp = opeType_C;
                break;
            case ')':
                mCurExpIdx = idx + 1;
                mCurVal.assign(")");
                mCurOpTyp = opeType_Reverse_C;
                break;
            default:
                if (idx < mExpression.size() - 1
                        && (std::tolower(mExpression[idx], lc) == 'o' && std::tolower(mExpression[idx+1], lc) == 'r')
                        && ((idx < mExpression.size() - 2 && mExpression[idx+2] == ' ') || idx >= mExpression.size() - 2)) {
                    mCurExpIdx = idx + 2;
                    mCurVal.assign("or");
                    mCurOpTyp = opeType_Group2;
                } else if (idx < mExpression.size() - 2
                           && (std::tolower(mExpression[idx], lc) == 'a' && std::tolower(mExpression[idx+1], lc) == 'n' && std::tolower(mExpression[idx+2], lc) == 'd')
                           && ((idx < mExpression.size() - 3 && mExpression[idx+3] == ' ') || idx >= mExpression.size() - 3)) {
                    mCurExpIdx = idx + 3;
                    mCurVal.assign("and");
                    mCurOpTyp = opeType_Group2;
                } else {
                    tmpIdx = idx;
                    for (; tmpIdx < mExpression.size(); tmpIdx++) {
                        if (mExpression[tmpIdx] == ' ' || mExpression[tmpIdx] == '=' || mExpression[tmpIdx] == '!' || mExpression[tmpIdx] == '>' || mExpression[tmpIdx] == '<'
                                || mExpression[tmpIdx] == '(' || mExpression[tmpIdx] == ')') {
                            break;
                        }
                    }

                    if (tmpIdx == idx) {
                        mCurExpIdx = tmpIdx + 1;
                    } else {
                        mCurExpIdx = tmpIdx;
                    }
                    mCurVal = mExpression.substr(idx, tmpIdx-idx);
                    mCurOpTyp = opeType_Element;
                }
                break;
            }

            break;
        }

        if (mCurVal.empty())
            mCurOpTyp = opeType_End;
    }

    bool stateElement() {
        bool ret{false};
        std::string curVal;
        mOpTypTrail.push_back(opeType_Element);
        mOpTypTrailPrecise.push_back(opeTypePrecise_Element);

        if (mOpTypTrailPrecise.size() < 2) {
            OpeNode* tmp = getOpeNode();
            if (mCStack.size() > 0) {
                tmp->CParentNext = mCStack[mCStack.size()-1];
            }
            tmp->pre = mCurNode;
            tmp->leftElement = mCurVal;
            mCurNode->next.push_back(tmp);
            mCurNode = tmp;
        } else {
            if (mOpTypTrailPrecise[mOpTypTrailPrecise.size()-2] == opeTypePrecise_i) {
                mCurNode->leftElement = mCurVal;
            } else if (mOpTypTrailPrecise[mOpTypTrailPrecise.size()-2] == opeTypePrecise_or) {
                OpeNode* tmp = getOpeNode();
                tmp->leftElement = mCurVal;
                tmp->brancheNode = true;
                if (mCStack.size() > 0) {
                    tmp->CParentNext = mCStack[mCStack.size()-1];
                }

                tmp->pre = mCurNode;
                mCurNode->branch.push_back(tmp);
                mCurNode = tmp;
            } else if (mOpTypTrailPrecise[mOpTypTrailPrecise.size()-2] == opeTypePrecise_and) {
                OpeNode* tmp = getOpeNode();
                tmp->leftElement = mCurVal;
                if (mCStack.size() > 0) {
                    tmp->CParentNext = mCStack[mCStack.size()-1];
                }

                tmp->pre = mCurNode;
                mCurNode->next.push_back(tmp);
                mCurNode = tmp;
            } else if (mOpTypTrail[mOpTypTrail.size()-2] == opeType_Group1) {
                mCurNode->rightElement = mCurVal;
            } else if (mOpTypTrailPrecise[mOpTypTrailPrecise.size()-2] == opeTypePrecise_C) {
                OpeNode* tmp = getOpeNode();
                tmp->leftElement = mCurVal;
                if (mCStack.size() > 0) {
                    tmp->CParentNext = mCStack[mCStack.size()-1];
                }

                tmp->pre = mCurNode;
                mCurNode->next.push_back(tmp);
                mCurNode = tmp;
            }
        }

        getToken();
        curVal = mCurVal;
        switch (mCurOpTyp) {
        case opeType_End:
            if (mTolerance > 0)
                ret = false;
            else
                ret = stateEnd();
            break;
        case opeType_Group1:
            if (mOpTypTrail.size() >= 2 && mOpTypTrail[mOpTypTrail.size() - 1] == opeType_Element && (mOpTypTrail[mOpTypTrail.size() - 2] == opeType_Group1 || mOpTypTrail[mOpTypTrail.size() - 2] == opeType_i))
                ret = false;
            else
                ret = stateOpeG1();
            break;
        case opeType_Group2:
            ret = stateOpeG2();
            break;
        case opeType_Reverse_C:
            if (mTolerance > 0)
                ret = stateRC();
            else
                ret = false;
            break;
        default:
            break;
        }

        if (!ret)
            //            cerr << "Invalid state after `" << curVal << "`" << endl;
            mErrMsg += " Invalid state after `" + curVal + "`\n";

        return ret;
    }

    bool stateC() {
        bool ret{false};
        std::string curVal;
        mOpTypTrail.push_back(opeType_C);
        mOpTypTrailPrecise.push_back(opeTypePrecise_C);

        mTolerance++;

        OpeNode* tmp = getOpeNode();
        tmp->opeSign.assign("(");
        if (mCStack.size())
            tmp->CParentNext = mCStack[mCStack.size()-1];

        mCStack.push_back(tmp);
        tmp->pre = mCurNode;

        if (mOpTypTrailPrecise.size() >= 2 && mOpTypTrailPrecise[mOpTypTrailPrecise.size()-2] == opeTypePrecise_or) {
            mCurNode->branch.push_back(tmp);
            tmp->brancheNode = true;
        } else {
            mCurNode->next.push_back(tmp);
        }
        mCurNode = tmp;

        getToken();
        curVal = mCurVal;
        switch (mCurOpTyp) {
        case opeType_Element:
            ret = stateElement();
            break;
        case opeType_C:
            ret = stateC();
            break;
        case opeType_i:
            ret = stateRi();
            break;
        default:
            break;
        }

        if (!ret)
            //            cerr << "Invalid state after `" << curVal << "`" << endl;
            mErrMsg += " Invalid state after `" + curVal + "`\n";

        return ret;
    }

    //get !
    bool stateRi() {
        bool ret{false};
        std::string curVal;
        mOpTypTrail.push_back(opeType_i);
        mOpTypTrailPrecise.push_back(opeTypePrecise_i);

        if (mOpTypTrailPrecise.size() < 2) {
            OpeNode* tmp = getOpeNode();
            tmp->UnLeft = true;
            tmp->pre = mCurNode;
            mCurNode->next.push_back(tmp);
            mCurNode = tmp;
        } else {
            if (mOpTypTrailPrecise[mOpTypTrailPrecise.size()-2] == opeTypePrecise_or) {
                OpeNode* tmp = getOpeNode();
                tmp->UnLeft = true;
                tmp->pre = mCurNode;
                tmp->brancheNode = true;
                if (mCStack.size())
                    tmp->CParentNext = mCStack[mCStack.size()-1];

                mCurNode->branch.push_back(tmp);
                mCurNode = tmp;
            } else {
                OpeNode* tmp = getOpeNode();
                tmp->UnLeft = true;
                tmp->pre = mCurNode;
                if (mCStack.size())
                    tmp->CParentNext = mCStack[mCStack.size()-1];

                mCurNode->next.push_back(tmp);
                mCurNode = tmp;
            }
        }

        getToken();
        curVal = mCurVal;
        switch (mCurOpTyp) {
        case opeType_Element:
            ret = stateElement();
            break;
        case opeType_C:
            ret = stateC();
            break;
        default:
            break;
        }

        if (!ret)
            //            cerr << "Invalid state after `" << curVal << "`" << endl;
            mErrMsg += " Invalid state after `" + curVal + "`\n";

        return ret;
    }

    bool stateRC() {
        bool ret{false};
        std::string curVal;
        mOpTypTrail.push_back(opeType_Reverse_C);
        mOpTypTrailPrecise.push_back(opeTypePrecise_C);
        mTolerance--;

        OpeNode* tmp = getOpeNode();
        mCurNode->next.push_back(tmp);
        tmp->pre = mCurNode;
        tmp->opeSign.assign(")");
        if (mCStack.size()) {
            //            tmp->next.push_back(mCStack[mCStack.size()-1]);
            tmp->CParentNext = mCStack[mCStack.size()-1];
            mCurNode = mCStack[mCStack.size()-1];

            mCStack.pop_back();
        } else {
            //some error occured. do nothing. error will be discovered in the follow process
        }

        getToken();
        curVal = mCurVal;
        switch (mCurOpTyp) {
        case opeType_End:
            if (mTolerance > 0)
                ret = false;
            else
                ret = stateEnd();
            break;

        case opeType_Group2:
            ret = stateOpeG2();
            break;

        case opeType_Reverse_C:
            if (mTolerance > 0)
                ret = stateRC();
            else
                ret = false;
            break;

        default:
            break;
        }

        if (!ret)
            //            cerr << "Invalid state after `" << curVal << "`" << endl;
            mErrMsg += " Invalid state after `" + curVal + "`\n";

        return ret;
    }

    bool stateOpeG1() {
        bool ret{false};
        std::string curVal;
        mOpTypTrail.push_back(opeType_Group1);
        /*
        opeTypePrecise_2_e,// ==
        opeTypePrecise_i_e,// !=
        opeTypePrecise_gt,// >
        opeTypePrecise_gt_e,// >=
        opeTypePrecise_lt,// <
        opeTypePrecise_lt_e,// <=,
        */
        if (mCurVal == "==") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_2_e);
        } else if (mCurVal == "!=") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_i_e);
        } else if (mCurVal == ">") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_gt);
        } else if (mCurVal == ">=") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_gt_e);
        } else if (mCurVal == "<") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_lt);
        } else if (mCurVal == "<=") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_lt_e);
        }

        mCurNode->opeSign = mCurVal;

        getToken();
        curVal = mCurVal;
        switch (mCurOpTyp) {
        case opeType_Element:
            ret = stateElement();
            break;
        default:
            break;
        }

        if (!ret)
            //            cerr << "Invalid state after `" << curVal << "`" << endl;
            mErrMsg += " Invalid state after `" + curVal + "`\n";

        return ret;
    }

    bool stateOpeG2() {
        bool ret{false};
        std::string curVal;
        mOpTypTrail.push_back(opeType_Group2);

        /*
        opeTypePrecise_or,// or
        opeTypePrecise_and// and
        */

        if (mCurNode->brancheNode)
            mCurNode = mCurNode->pre;
        if (mCurVal == "or") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_or);
        } else if (mCurVal == "and") {
            mOpTypTrailPrecise.push_back(opeTypePrecise_and);
        }

        getToken();
        curVal = mCurVal;
        switch (mCurOpTyp) {
        case opeType_Element:
            ret = stateElement();
            break;
        case opeType_C:
            ret = stateC();
            break;
        case opeType_i:
            ret = stateRi();
            break;
        default:
            break;
        }

        if (!ret)
            //            cerr << "Invalid state after `" << curVal << "`" << endl;
            mErrMsg += " Invalid state after `" + curVal + "`\n";

        return ret;
    }

    bool stateEnd() {
        return true;
    }

    OpeNode* getOpeNode() {
        mDeleteVec.push_back(new OpeNode);
        return mDeleteVec[mDeleteVec.size()-1];
    }

    inline bool runExpRecur(OpeNode* node, void* param) {
        bool ret{false};
        bool bRet{false};
        size_t nextIdx{0};

        if (node == nullptr || node->opeSign == ")")
            return true;

        //if either branch or self has true, the current node can be passed
        for (auto bNode: node->branch) {
            if (runExpRecur(bNode, param)) {
                bRet = true;
                break;
            }
        }
        ret = bRet;
        if (node->opeSign == "(") {
            if (!bRet)
                ret = true;
        } else {
            if (!bRet)
                if (!node->leftElement.empty())
                    ret = mOpeCallBack(node->UnLeft, node->leftElement, node->opeSign, node->UnRight, node->rightElement, param);
        }
        if (!ret)
            return false;


        if (bRet && (node->opeSign == "("))
            nextIdx = 1;

        for (; nextIdx < node->next.size(); nextIdx++) {
            if (!runExpRecur(node->next[nextIdx], param))
                return false;
        }

        return true;
    }
};
#endif // EXPPARSER_HPP
