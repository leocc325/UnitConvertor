#ifndef UNITCONVERTOR_HPP
#define UNITCONVERTOR_HPP

#include <string>
#include <cmath>

class ValuePack;
struct ValuePackProperty;
namespace UnitConvertor
{
    enum DecimalRatio{Nano,Micro,Milli,One,Kilo,Mega,Giga,RatioNum};

    static const std::string DecimalRatioString[RatioNum] = {"n" , "u" , "m" , "" , "k" , "M" , "G"};

    enum DecimalUnit{Null,Freq,Time,Ampl,Voltage,Current,Phase,UnitNum};

    static const std::string DecimalUnitString[UnitNum] = {"" , "Hz" , "s" , "Vpp" , "V" , "A" , "°"};

    ///这个函数返回一个单位对应的属性:属性包括这个单位所对应的最大数量级、最小数量级、单位枚举
    static const ValuePackProperty generateValuePackProperty(DecimalUnit unit) noexcept;

    ///将一个字符串转换为数据包
    ValuePack fromString(const std::string& target);

    ///将当前数据的数量级转换为newRatio表示的数据
    ValuePack convertTo(ValuePack pack, DecimalRatio newRatio);

    ///将当前数据字符串的数量级转换为newRatio表示的数据
    ValuePack convertTo(const std::string& str, DecimalRatio newRatio);

    ///将数值自动转换为一个恰当单位表示的数值(1～999之间的值)
    ValuePack proper(ValuePack pack);

    ///将字符串自动转换为一个恰当单位表示的数值(1～999之间的值)
    ValuePack proper(const std::string& str);
};

namespace UC = UnitConvertor;

///描述当前单位的最大数量级和最小数量级
struct ValuePackProperty
{
    UnitConvertor::DecimalRatio maxRatio = UnitConvertor::One;
    UnitConvertor::DecimalRatio minRatio = UnitConvertor::One;
    UnitConvertor::DecimalUnit unit = UnitConvertor::Null;
};

class ValuePack
{
    //这个类只有数值成员变量,无需自定义拷贝和移动函数,太懒了不想重载+=、-=、*=、/=这些运算符,有需要的时候再加
public:
    ValuePack();

    ValuePack(double value,UC::DecimalRatio ratio,UC::DecimalUnit unit);

    bool operator==(const ValuePack& other) const noexcept
    {
        return std::abs(this->m_Value - other.m_Value) < 1e-10
            &&this->m_Ratio == other.m_Ratio
            && this->m_Property.unit == other.m_Property.unit;
    }

    bool operator!=(const ValuePack& other) const noexcept{
        return !(*this == other);
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
            tmpPack = UnitConvertor::convertTo(pack,this->m_Ratio);
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
            tmpPack = UnitConvertor::convertTo(pack,this->m_Ratio);
            return ValuePack(this->m_Value - tmpPack.m_Value,this->m_Ratio,this->m_Property.unit);
        }
        return tmpPack;
    }

    operator double() const noexcept { return m_Value; }

    void setValue(double value) noexcept{
        this->m_Value = value;
    }

    double value() const noexcept{
        return this->m_Value;
    }

    void setRatio(UC::DecimalRatio ratio) noexcept{
        this->m_Ratio = ratio;
    }

    UC::DecimalRatio ratio() const noexcept{
        return this->m_Ratio;
    }

    ValuePackProperty property() const noexcept{
        return this->m_Property;
    }

    std::string toString(){
        return std::to_string(m_Value) + UC::DecimalRatioString[m_Ratio] + UC::DecimalUnitString[m_Property.unit];
    }

    std::string toFormatString(){
        return std::to_string(m_Value) + UC::DecimalRatioString[m_Ratio] + UC::DecimalUnitString[m_Property.unit];
    }

    std::string toScientificString(){
        return std::to_string(m_Value) + UC::DecimalRatioString[m_Ratio] + UC::DecimalUnitString[m_Property.unit];
    }

    std::size_t toInt() const noexcept {
        return std::round(m_Value);
    }
private:
    double m_Value = 0;
    UC::DecimalRatio m_Ratio = UC::One;
    ValuePackProperty m_Property;
};

#endif // UNITCONVERTOR_HPP
