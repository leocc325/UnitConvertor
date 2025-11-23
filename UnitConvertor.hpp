#ifndef UNITCONVERTOR_HPP
#define UNITCONVERTOR_HPP

#include <string>
#include <cmath>

class ValuePack;
struct UnitProperty;
namespace UnitConvertor
{
enum DecimalRatio{Nano,Micro,Milli,One,Kilo,Mega,Giga,RatioNum};

static const std::string DecimalRatioString[RatioNum] = {"n" , "u" , "m" , "" , "k" , "M" , "G"};

enum DecimalUnit{Null,Freq,Time,Ampl,Voltage,Current,Phase,SampRate,VolArea,UnitNum};

static const std::string DecimalUnitString[UnitNum] = {"" , "Hz" , "s" , "Vpp" , "V" , "A" , "°","Sa/s","V*s"};

///这个函数返回一个单位对应的属性:属性包括这个单位所对应的最大数量级、最小数量级、单位枚举
const UnitProperty generateUnitProperty(DecimalUnit unit) noexcept;

///将一个字符串转换为数据包,如果在调用这个函数的时候指定单位类型执行效率将会更高
ValuePack fromString(const std::string& target,DecimalUnit unit = DecimalUnit::UnitNum);

///将当前数据的数量级转换为newRatio表示的数据
ValuePack ratioTo(ValuePack pack, DecimalRatio newRatio);

///将当前数据字符串的数量级转换为newRatio表示的数据
ValuePack ratioTo(const std::string& str, DecimalRatio newRatio);

///将数值自动转换为一个恰当单位表示的数值(1～999之间的值)
ValuePack proper(ValuePack pack);

///将字符串自动转换为一个恰当单位表示的数值(1～999之间的值)
ValuePack proper(const std::string& str);

///将ValuePack转换为字符串,不控制格式
std::string toString(const ValuePack& pack);

///将ValuePack转换为字符串,当fixedDecimal为true时精确到小数点后precision位,否则位保留fixedDecimal位有效数字
std::string toFormatString(const ValuePack& pack,int precision,bool fixedDecimal = true);

///将ValuePack转换为字符串同时确保整数部分和小数部分长度固定,长度不足时填充给定字符
std::string toFormatString(const ValuePack& pack,int totalLeng,int decimalLen,char fill = '0');

///将ValuePack转换为科学计数法的字符串,不控制格式
std::string toScientificString(const ValuePack& pack);

///将ValuePack转换为科学计数法的字符串,小数点后面保留precision位
std::string toScientificString(const ValuePack& pack,int precision);

///将ValuePack转换为整数
long long toInt(const ValuePack& pack);
};
namespace Uc =  UnitConvertor;

///描述当前单位的最大数量级和最小数量级以及每一个数量级之间的进制
struct UnitProperty
{
    UnitConvertor::DecimalRatio maxRatio = UnitConvertor::One;
    UnitConvertor::DecimalRatio minRatio = UnitConvertor::One;
    UnitConvertor::DecimalUnit unit = UnitConvertor::Null;
    double Exp = 1000;
    //把进制作为成员变量而不是全局变量以以获得更高的灵活性,比如kV和V之间的进制是1000,km²和m²的进制1000000
};

class ValuePack
{
    //这个类只有数值成员变量,无需自定义拷贝和移动函数,太懒了不想重载+=、-=、*=、/=这些运算符,有需要的时候再加
public:
    ValuePack(){}

    ValuePack(double value,UnitConvertor::DecimalRatio ratio,UnitConvertor::DecimalUnit unit)
    {
        this->m_Value = value;
        this->m_Ratio = ratio;
        this->m_Property = UnitConvertor::generateUnitProperty(unit);
    }

    ///可以直接与数值类型变量相加
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value,ValuePack>::type
    operator + (T value) const noexcept{
        return ValuePack(this->m_Value+value, this->ratio(), this->m_Property.unit);
    }

    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value,ValuePack>::type
    operator - (T value) const noexcept{
        return ValuePack(this->m_Value-value, this->ratio(), this->m_Property.unit);
    }

    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value,ValuePack>::type
    operator * (T value) const noexcept{
        return ValuePack(this->m_Value*value, this->ratio(), this->m_Property.unit);
    }

    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value,ValuePack>::type
    operator / (T value) const noexcept{
        return ValuePack(this->m_Value*value, this->ratio(), this->m_Property.unit);
    }

    ///只有单位类型一致的变量可以互相加,相加之后数量级以当前对象为准,如果单位不一致返回一个缺省的对象
    ValuePack operator + (const ValuePack& pack) const noexcept
    {
        ValuePack tmpPack{};
        if(pack.m_Property.unit == this->m_Property.unit)
        {
            tmpPack = UnitConvertor::ratioTo(pack,this->m_Ratio);
            return ValuePack(this->m_Value + tmpPack.m_Value,this->m_Ratio,this->m_Property.unit);
        }
        return tmpPack;
    }

    ///只有单位类型一致的变量可以互相减,相减之后数量级以当前对象为准,如果单位不一致返回一个缺省的对象
    ValuePack operator - (const ValuePack& pack) const noexcept
    {
        ValuePack tmpPack{};
        if(pack.m_Property.unit == this->m_Property.unit)
        {
            tmpPack = UnitConvertor::ratioTo(pack,this->m_Ratio);
            return ValuePack(this->m_Value - tmpPack.m_Value,this->m_Ratio,this->m_Property.unit);
        }
        return tmpPack;
    }

    ValuePack& operator = (double value) noexcept
    {
        m_Value = value;
        return *this;
    }

    bool operator==(const ValuePack& other) const noexcept
    {
        return std::abs(this->m_Value - other.m_Value) < 1e-10
            &&this->m_Ratio == other.m_Ratio
            && this->m_Property.unit == other.m_Property.unit;
    }

    bool operator!=(const ValuePack& other) const noexcept{
        return !(*this == other);
    }

    operator double() const noexcept { return m_Value; }

    double value() const noexcept{return this->m_Value;}

    UnitConvertor::DecimalRatio ratio() const noexcept{return this->m_Ratio;}

    UnitConvertor::DecimalUnit unit() const noexcept{return this->m_Property.unit;}

    UnitProperty property() const noexcept{return this->m_Property;}

private:
    double m_Value = 0;
    UnitConvertor::DecimalRatio m_Ratio = UnitConvertor::One;
    UnitProperty m_Property;
};

#endif // UNITCONVERTOR_HPP
