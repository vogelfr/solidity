/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libyul/optimiser/VarNameCleaner.h>
#include <libyul/AsmData.h>
#include <string>
#include <iterator>
#include <algorithm>
#include <cctype>

using namespace yul;
using namespace std;

// TODO: ensure we don't rewrite external or reserved names

// TODO: ensure the following transforms
// - input: a, a_1, a_1_2
//	 output: a, a_1, a_2
// - input: a, a_1, a_1_2, a_2
//   output: a, a_1, a_2, a_3
// - input: a_15, a_17
//   output: a, a_2

void VarNameCleaner::operator()(VariableDeclaration& _varDecl)
{
	for (TypedName& typedName: _varDecl.variables)
	{
		if (auto newName = makeCleanName(typedName.name.str()))
		{
			printf("VariableDeclaration: %s -> %s\n", typedName.name.str().c_str(), newName->c_str());
			typedName.name = YulString{*newName};
		}
		else
			printf("VariableDeclaration: %s\n", typedName.name.str().c_str());
	}

	ASTModifier::operator()(_varDecl);
}

void VarNameCleaner::operator()(Identifier& _identifier)
{
	if (auto newName = getCleanName(_identifier.name.str()))
	{
		printf("Identifier: %s -> %s\n", _identifier.name.str().c_str(), newName->c_str());
		_identifier.name = YulString{*newName};
	}
	else
	{
		printf("Identifier: %s\n", _identifier.name.str().c_str());
	}
}

boost::optional<string> VarNameCleaner::makeCleanName(string const& _name)
{
	printf("makeCleanName: %s\n", _name.c_str());
	// make identifier clean, remember new name for reuse, keep them unique and simple
	auto n = m_usedNames.find(_name);
	if (n != m_usedNames.end())
	{
		// if (_name != n->second)
			// mapping already registered, and name was changed
			return {n->second};
	}

	auto c = m_useCounts.find(_name);
	if (c != m_useCounts.end())
	{
		// no mapping registered for that name yet, but basename already used in the past
		string newName = _name + "_" + to_string(c->second);
		m_usedNames[_name] = newName;
		c->second++;
		return {newName};
	}

	// Generate a "clean" name out of the input name, by:
	// - stripping potential suffixes (_NUMBERS) out of the name, iff they potentially new name hasn't been "used" already.

	if (auto newName = stripSuffix(_name))
	{
		if (!m_usedNames.count(*newName))
		{
			// strip suffix
			m_useCounts[_name] = 1;
			m_usedNames[_name] = *newName;
			return newName;
		}
	}

	// name isn't yet used and we're not going to rewrite it
	m_useCounts[_name] = 1;
	m_usedNames[_name] = _name;
	return boost::none;
}

boost::optional<std::string> VarNameCleaner::stripSuffix(string const& _name)
{
	// valid suffixes are (_[0-9]+)+
	auto isValidSuffix = [](string const& _name, size_t i) -> bool
	{
		return i + 1 < _name.size() && all_of(next(cbegin(_name), i),
											  cend(_name),
											  [](auto c) -> bool { return c = '_' || isdigit(c); });
	};

	for (size_t i = _name.find('_'); i != _name.npos; ++i)
		if (isValidSuffix(_name, i))
			return _name.substr(0, i);

	return boost::none;
}

boost::optional<string> VarNameCleaner::getCleanName(string const& _name) const
{
	auto n = m_usedNames.find(_name);
	if (n != m_usedNames.end())
	{
		if (_name != n->second)
			// mapping already registered, and name was changed
			return {n->second};
		else
			// mapping already registered, no name change required
			return boost::none;
	}
	// no mapping registered for that name
	return boost::none;
}
