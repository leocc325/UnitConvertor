#include "UnitConvertor.hpp"

#include <regex>
#include <set>
#include <unordered_map>
#include <mutex>

using namespace UnitConvertor;

static std::once_flag ratioMapOnce;
static std::once_flag unitMapOnce;
static std::unordered_map<std::string,std::size_t> GlobalRatioMap;
static std::unordered_map<std::string,std::size_t> GlobalUnitMap;
static const double Ratio = 1000;

void initRatioMap()
{
    for(int i = 0; i < UC::RatioNum; i++)
    {
        GlobalRatioMap.emplace(DecimalRatioString[i],i);
    }
}

void initUnitMap()
{
    for(int i = 0; i < UC::UnitNum; i++)
        {
            GlobalUnitMap.emplace(DecimalUnitString[i],i);
        }
}

///获取比例字符串和枚举值的映射关系
static const std::unordered_map<std::string,std::size_t>& ratioMap()
{
    std::call_once(ratioMapOnce,initRatioMap);
    return GlobalRatioMap;
}

///获取单位字符串和枚举值的映射关系
static const std::unordered_map<std::string,std::size_t>& unitMap()
{
    std::call_once(unitMapOnce,initUnitMap);
    return GlobalUnitMap;
}

///转义正则表达式特殊字符
const std::string escapeRegex(const std::string &str)
{
    std::string result;
    for (char c : str)
    {
        if (c == '.' || c == '*' || c == '+' || c == '?' || c == '^' ||
                c == '$' || c == '|' || c == '(' || c == ')' || c == '[' ||
                c == ']' || c == '{' || c == '}' || c == '\\')
        {
            result += '\\';
        }
        result += c;
    }
    return result;
}

///自定义比较函数对象
struct LengthCompare
{
    bool operator()(const std::string& a, const std::string& b) const {
        if (a.length() != b.length())
        {
            return a.length() > b.length(); // 长度长的在前
        }
        return a < b; // 长度相同时按字典序排列
    }
};

///生成一个可以自动匹配string数组中各个字符串的正则表达式
const std::string generateRegexString(const std::string *array, const std::size_t arraySize)
{
    std::set<std::string, LengthCompare> stringSet;
    // 按长度降序排序，确保长匹配优先
    for (size_t i = 0; i < arraySize; ++i)
    {
        if (!array[i].empty())
        {
            stringSet.insert(array[i]);
        }
    }

    std::ostringstream pattern;
    pattern << "(";

    // 构建正则表达式字符串
    for (auto it = stringSet.begin(); it != stringSet.end(); ++it)
    {
        if (it != stringSet.begin())
            pattern << "|";

        pattern << escapeRegex(*it);
    }
    pattern << ")?";

    return pattern.str();
}

///返回一个查找比例字符串的正则表达式,根据比例字符串数组自动生成一个用于查找比例字符串的正则表达式,更新比例字符串数组之后正则表达式会自动更新
const std::regex& ratioRegex()
{
    static const std::regex instance(generateRegexString(DecimalRatioString,RatioNum),std::regex_constants::icase);
    return instance;
}

///返回一个查找单位字符串的正则表达式,根据单位字符串数组自动生成一个用于查找单位字符串的正则表达式,更新单位字符串数组之后正则表达式会自动更新
const std::regex& unitRegex()
{
    static const std::regex instance(generateRegexString(DecimalUnitString,UnitNum),std::regex_constants::icase);
    return instance;
}

///返回查找数值字符串的正则表达式[暂时还不支持科学计数法]
const std::regex& decimalRegex()
{
    static const std::regex  reg(R"(-?\d*\.?\d+)",std::regex_constants::icase);
    return reg;
}

///通过三个字符串生成数据包
ValuePack generateValuePack(const std::string& valueStr,const std::string& ratioStr,const std::string& unitStr)
{
    double value = std::stod(valueStr);
    DecimalRatio ratio;
    DecimalUnit unit ;

    try {
        ratio = static_cast<DecimalRatio>(ratioMap().at(ratioStr));
    } catch (std::out_of_range) {
        ratio = UnitConvertor::One;
    }

    try {
        unit = static_cast<DecimalUnit>(unitMap().at(unitStr));
    } catch (std::out_of_range) {
        unit = UnitConvertor::Null;
    }

    return ValuePack(value,ratio,unit);
}

const ValuePackProperty UnitConvertor::generateValuePackProperty(DecimalUnit unit) noexcept
{
    switch (unit)
    {
    case Freq:return ValuePackProperty{Giga,One,Freq} ;break;
    case Time:return ValuePackProperty{One,Nano,Freq} ;break;
    case Ampl:return ValuePackProperty{Kilo,Micro,Ampl} ;break;
    case Voltage:return ValuePackProperty{Kilo,Micro,Voltage} ;break;
    case Current:return ValuePackProperty{Kilo,Micro,Current} ;break;
    case Phase:return ValuePackProperty{One,One,Phase} ;break;
    default:return ValuePackProperty{} ;
    }
}

ValuePack UnitConvertor::fromString(const std::string &target)
{
    std::string valueStr = "0";
    std::string ratioStr;
    std::string unitStr;

    const char* begin = target.data();
    const char* end = begin + target.length();

    //先查找数字对应的字符串
    const std::regex& decimalReg = decimalRegex();
    std::cregex_iterator decimalBeg(begin,end,decimalReg);
    std::cregex_iterator decimalEnd;
    if(std::distance(decimalBeg,decimalEnd) == 1)
    {
        //如果查找到的数字字符串不等于1(多余或者少于1都认为是错误的,此时让值保持为默认值0)
        valueStr = decimalBeg->str();
        begin += decimalBeg->size();//移动指针缩小查找范围
    }

    //再查找单位对应的字符串
    const std::regex& unitReg = unitRegex();
    std::cregex_iterator unitBeg(begin,end,unitReg);
    std::cregex_iterator unitEnd;
    if(std::distance(unitBeg,unitEnd) == 1)
    {
        //允许单位为空,所以当单位查找失败时字符串会保持为空
        unitStr = unitBeg->str();
        end -= unitBeg->size();//再次缩写查找范围
    }

    const std::regex& ratioReg = ratioRegex();
    std::cregex_iterator ratioBeg(begin,end,ratioReg);
    std::cregex_iterator ratioEnd;
    if(std::distance(ratioBeg,ratioEnd) == 1)
    {
        ratioStr = unitBeg->str();
    }

    //最后将查找到的三个字符串转换为数据包
    return generateValuePack(valueStr,ratioStr,unitStr);
}

ValuePack UnitConvertor::convertTo(ValuePack pack, DecimalRatio newRatio)
{
    //将数量级限制到这个数据包的单位所对应的数量级范围内
    newRatio = std::min(pack.property().maxRatio,newRatio);
    newRatio = std::max(pack.property().minRatio,newRatio);

    double value = pack.value() * Ratio * (pack.ratio(),newRatio);
    return ValuePack(value,newRatio,pack.property().unit);
}

ValuePack UnitConvertor::convertTo(const std::string &str, DecimalRatio newRatio)
{
    ValuePack pack = UnitConvertor::fromString(str);
    return UnitConvertor::convertTo(pack,newRatio);
}

ValuePack UnitConvertor::proper(ValuePack pack)
{
    if(pack.value() > 1000)
        return proper( ValuePack(pack.value()/1000, DecimalRatio(pack.ratio()-1), pack.property().unit) );
    else if(pack.value() < 1)
        return proper( ValuePack(pack.value()*1000, DecimalRatio(pack.ratio()+1), pack.property().unit) );
    else
        return pack;
}

ValuePack UnitConvertor::proper(const std::string &str)
{
    ValuePack pack = UC::fromString(str);
    return proper(pack);
}

/**********************************************************************
* @name:   ValuePack
* @brief:   以下是ValuePack类的成员函数定义
* @param:   无
* @return:  无
* @author:  lcc
* @date:    2025/10/29
* @notes:   无
***********************************************************************/

ValuePack::ValuePack()
{
    ValuePack pack;
    pack+10;
}

ValuePack::ValuePack(double value, DecimalRatio ratio, DecimalUnit unit)
{
    this->m_Value = value;
    this->m_Ratio = ratio;
    this->m_Property = UnitConvertor::generateValuePackProperty(unit);
}
