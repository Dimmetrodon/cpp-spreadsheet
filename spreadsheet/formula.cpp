#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) 
{
    return output << "#DIV/0!";
}

namespace 
{
class Formula : public FormulaInterface 
{
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) 
    :ast_(ParseFormulaAST(expression))
    {
    }

    Value Evaluate(const SheetInterface& sheet) const override 
    {
        const std::function<double(Position)>& args = [&sheet](Position pos) 
        {
            if (!pos.IsValid()) 
            {
                throw FormulaError(FormulaError::Category::Ref); 
            }
            if (sheet.GetCell(pos) == nullptr) 
            {
                return 0.0;
            }

            CellInterface::Value value = sheet.GetCell(pos)->GetValue();

            if (std::holds_alternative<double>(value)) 
            {
                return std::get<double>(value);
            }
            if (std::holds_alternative<FormulaError>(value)) 
            {
                throw std::get<FormulaError>(value);
            }
            if (std::holds_alternative<std::string>(value)) 
            {
                //return std::stod(std::get<std::string>(value));
                if (std::get<std::string>(value).empty()) 
                {
                    return 0.0;
                }
                if (std::get<std::string>(value).find_first_not_of("0123456789"s) != std::string::npos) 
                {
                    throw FormulaError(FormulaError::Category::Value);
                }
                return stod(std::get<std::string>(value));
            }
            else
            {
                throw FormulaError(std::get<FormulaError>(value));
            }
        };

        try 
        {
            return ast_.Execute(args);
        } 
        catch (const FormulaError& err) 
        {
            return err;
        }
    }

    std::string GetExpression() const override 
    {
        std::stringstream ss;
        ast_.PrintFormula(ss);
        return ss.str();
    }

    std::vector<Position> GetReferencedCells() const override
    {
        std::set<Position> result;
        for (const auto& cell : ast_.GetCells()) 
        {
            if (cell.IsValid()) 
            {
                result.insert(cell);
            }
        }
        return {result.begin(), result.end()};
    }

private:
    FormulaAST ast_;
};
}  // namespace

FormulaError::FormulaError(Category category)
    :category_(category)
{
}

FormulaError::Category FormulaError::GetCategory() const
{
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const
{
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const
{
    switch(category_)
    {
        case (Category::Div0):
        {
            return "#DIV/0!"s;
        }
        case(Category::Ref):
        {
            return "#REF!"s;
        }
        case(Category::Value):
        {
            return "#VALUE!"s;
        }
        default:
        {
            return{};
        }
    }
}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) 
{
    try 
    {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) 
    {
        throw FormulaException("incorrect formula"s);
    }
}