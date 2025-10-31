#include "UnitConvertor.hpp"

#include <iomanip>
#include <regex>
#include <set>
#include <unordered_map>
#include <mutex>

using namespace UnitConvertor;

static std::once_flag ratioMapOnce;
static std::once_flag unitMapOnce;
static std::unordered_map<std::string,std::size_t> GlobalRatioMap;
static std::unordered_map<std::string,std::size_t> GlobalUnitMap;

std::string toLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),[](unsigned char c) { return std::tolower(c); });
    return result;
}

void initRatioMap()
{
    for(int i = 0; i < UnitConvertor::RatioNum; i++)
    {
        GlobalRatioMap.emplace(DecimalRatioString[i],i);
    }
}

void initUnitMap()
{
    for(int i = 0; i < UnitConvertor::UnitNum; i++)
    {
        GlobalUnitMap.emplace(toLower(DecimalUnitString[i]),i);//查找单位时转换为小写
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

    std::string pattern;
    pattern.append("(");

    // 构建正则表达式字符串
    for (auto it = stringSet.begin(); it != stringSet.end(); ++it)
    {
        if (it != stringSet.begin())
            pattern.append("|");

        pattern.append(escapeRegex(*it));
    }
    pattern.append( ")+");
    return pattern;
}

///返回一个查找比例字符串的正则表达式,根据比例字符串数组自动生成一个用于查找比例字符串的正则表达式,更新比例字符串数组之后正则表达式会自动更新(quf)
const std::regex& ratioRegex()
{
    //数量级必须区分大小写,因为m和M会重复
    static const std::regex instance(generateRegexString(DecimalRatioString,RatioNum)/*,std::regex_constants::icase*/);
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
    //如果支持正则表达式之后还要修改数值转换部分代码,正则表达式匹配科学计数法会产生两部分结果
    //static const std::regex reg(R"(^[-+]?(\d+\.\d+|\d+\.?|\.\d+)([eE][-+]?\d+)?$)");

    static const std::regex  reg(R"(-?\d*\.?\d+)",std::regex_constants::icase);
    return reg;
}

///判断字符串小数部分是否全部都是0,如果小数点后面全部都是0则删除
///这样就不需要区分ValuePack的m_Value原本是浮点值还是整数值,避免整数值传入ValuePack转字符串之后后面有0
const std::regex& fractionalRegex()
{
    static const std::regex  reg(R"(\.0+$)");
    return reg;
}

///判断科学计数法字符串的小数部分是否全部为0
const std::regex& scientificRegex()
{
    static const std::regex  reg(R"(\.0+e)",std::regex_constants::icase);
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
        unit = static_cast<DecimalUnit>(unitMap().at(toLower(unitStr)));//查找单位时转换为小写
    } catch (std::out_of_range) {
        unit = UnitConvertor::Null;
    }

    return ValuePack(value,ratio,unit);
}

const UnitProperty UnitConvertor::generateUnitProperty(DecimalUnit unit) noexcept
{
    switch (unit)
    {
    case Freq:return UnitProperty{Giga,One,Freq,1000} ;break;
    case Time:return UnitProperty{One,Nano,Time,1000} ;break;
    case Ampl:return UnitProperty{Kilo,Micro,Ampl,1000} ;break;
    case Voltage:return UnitProperty{Kilo,Micro,Voltage,1000} ;break;
    case Current:return UnitProperty{Kilo,Micro,Current,1000} ;break;
    case Phase:return UnitProperty{One,One,Phase,1000} ;break;
    default:return UnitProperty{} ;
    }
}

ValuePack UnitConvertor::fromString(const std::string &target)
{
    std::string valueStr = "0";
    std::string ratioStr;
    std::string unitStr;

    const char* begin = target.data();
    const char* end = begin + target.length();

    //先查找数字对应的字符串,允许查找结果为空,查找失败时字符串会保持为空
    const std::regex& decimalReg = decimalRegex();
    std::cregex_iterator decimalBeg(begin,end,decimalReg);
    std::cregex_iterator decimalEnd;
    if(std::distance(decimalBeg,decimalEnd) == 1)//如果查找到的数字字符串不等于1(多余或者少于1都认为是错误的,此时让值保持为默认值0)
    {
        valueStr = decimalBeg->str();
        begin += decimalBeg->str().length();//移动指针缩小查找范围
    }

    //再查找单位对应的字符串
    const std::regex& unitReg = unitRegex();
    std::cregex_iterator unitBeg(begin,end,unitReg);
    std::cregex_iterator unitEnd;
    if(std::distance(unitBeg,unitEnd) == 1)//如果查找到的数字字符串不等于1(多余或者少于1都认为是错误的,此时让值保持为默认值0)
    {
        unitStr = unitBeg->str();
        end -= unitBeg->str().length();//再次缩写查找范围
    }

    const std::regex& ratioReg = ratioRegex();
    std::cregex_iterator ratioBeg(begin,end,ratioReg);
    std::cregex_iterator ratioEnd;
    if(std::distance(ratioBeg,ratioEnd) == 1)//如果查找到的数字字符串不等于1(多余或者少于1都认为是错误的,此时让值保持为默认值0)
    {
        ratioStr = ratioBeg->str();
    }

    //最后将查找到的三个字符串转换为数据包
    return generateValuePack(valueStr,ratioStr,unitStr);
}

ValuePack UnitConvertor::ratioTo(ValuePack pack, DecimalRatio newRatio)
{
    //将数量级限制到这个数据包的单位所对应的数量级范围内
    newRatio = std::min(pack.property().maxRatio,newRatio);
    newRatio = std::max(pack.property().minRatio,newRatio);

    double value = pack.value() * std::pow(pack.property().Exp , pack.ratio() - newRatio);
    return ValuePack(value,newRatio,pack.property().unit);
}

ValuePack UnitConvertor::ratioTo(const std::string &str, DecimalRatio newRatio)
{
    ValuePack pack = UnitConvertor::fromString(str);
    return UnitConvertor::ratioTo(pack,newRatio);
}

ValuePack UnitConvertor::proper(ValuePack pack)
{
    if(pack.value() > 1000)
        return proper( ValuePack(pack.value()/pack.property().Exp, DecimalRatio(pack.ratio() + 1), pack.property().unit) );
    else if(pack.value() < 1)
        return proper( ValuePack(pack.value()*pack.property().Exp, DecimalRatio(pack.ratio() - 1), pack.property().unit) );
    else
        return pack;
}

ValuePack UnitConvertor::proper(const std::string &str)
{
    ValuePack pack = UnitConvertor::fromString(str);
    return proper(pack);
}

std::string UnitConvertor::toString(const ValuePack &pack)
{
    std::ostringstream oss;
    oss  <<std::fixed << pack.value();
    std::string s = std::regex_replace(oss.str(),fractionalRegex(),"");
    return s + " " + UnitConvertor::DecimalRatioString[pack.ratio()] + UnitConvertor::DecimalUnitString[pack.property().unit];
}

std::string UnitConvertor::toFormatString(const ValuePack &pack, int precision,bool fixedDecimal )
{
    std::ostringstream oss;
    if(fixedDecimal)
        oss << std::fixed << std::setprecision(precision) << pack.value();
    else
        oss << std::setprecision(precision) << pack.value();
    std::string s = std::regex_replace(oss.str(),fractionalRegex(),"");
   return s + " " + UnitConvertor::DecimalRatioString[pack.ratio()] + UnitConvertor::DecimalUnitString[pack.property().unit];
}

std::string UnitConvertor::toFormatString(const ValuePack &pack, int totalLeng, int decimalLen, char fill)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimalLen) << std::setw(totalLeng) << std::setfill(fill) << pack.value();
    std::string s = std::regex_replace(oss.str(),fractionalRegex(),"");
    return s + " " + UnitConvertor::DecimalRatioString[pack.ratio()] + UnitConvertor::DecimalUnitString[pack.property().unit];
}

std::string UnitConvertor::toScientificString(const ValuePack &pack)
{
    std::ostringstream oss;
    oss << std::scientific  << pack.value();
    std::string s = std::regex_replace(oss.str(),scientificRegex(),"e");
    return s + " " + UnitConvertor::DecimalRatioString[pack.ratio()] + UnitConvertor::DecimalUnitString[pack.property().unit];
}

std::string UnitConvertor::toScientificString(const ValuePack &pack, int precision)
{
    std::ostringstream oss;
    oss << std::scientific  << std::setprecision(precision) << pack.value();
    std::string s = std::regex_replace(oss.str(),scientificRegex(),"e");
    return s + " " + UnitConvertor::DecimalRatioString[pack.ratio()] + UnitConvertor::DecimalUnitString[pack.property().unit];
}
long long UnitConvertor::toInt(const ValuePack& pack)
{
    return std::llround(pack.value());
}
