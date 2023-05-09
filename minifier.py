import re

def fetch_tokens(content: str) -> list:
    """Fetches all the tokens from the source code. A token is a word, number, whitespace, symbol, etc."""
    return [t for t in re.split('([^\w])', content) if t]

assert fetch_tokens('int main() { return 0; }') == ['int', ' ', 'main', '(', ')', ' ', '{', ' ', 'return', ' ', '0', ';', ' ', '}']
assert fetch_tokens('hehe = 1;') == ['hehe', ' ', '=', ' ', '1', ';']
assert fetch_tokens('a_very_long_variable_name') == ['a_very_long_variable_name']
assert fetch_tokens("test\t\ntest") == ["test", "\t", "\n", "test"]


def is_name(token: str) -> bool:
    """A name of /something/. Either a variable, function, class, etc."""
    return token and (token[0].isalpha() or token.startswith('_'))

assert is_name('a')
assert is_name('a_')
assert is_name('a_1')
assert is_name('__AA')
assert is_name('variable')
assert is_name('my_favorite_number_100000')
assert is_name('int')
assert not is_name('123')
assert not is_name('1a2b3c')
assert not is_name('')
assert not is_name(' ')
assert not is_name('\n')

TYPES = set(['int'])
def is_type(token: str) -> bool:
    """Returns whether the token is a CPP type used in the engine."""
    return token in TYPES


def renameable(token: str) -> bool:
    """Returns whether the token is renameable, i.e names that aren't types."""
    return not is_type(token) and is_name(token)

assert renameable('abcdefg')
assert renameable('main')
assert not renameable('int')
assert not renameable('1000')


def attach_eligble(token: str) -> bool:
    """Returns whether a token is eligble to be attached together to save whitespace."""
    return token and not (is_name(token) or token.isnumeric())

assert attach_eligble(')')
assert attach_eligble('+')
assert attach_eligble('{')
assert not attach_eligble('a_name')
assert not attach_eligble('1')
assert not attach_eligble('main')


def attachble_tokens(first: str, second: str) -> bool:
    """Returns whether two tokens can be attached together to save whitespace."""
    return attach_eligble(first) or attach_eligble(second)

assert attachble_tokens('1', '+')
assert attachble_tokens('{', '}')
assert attachble_tokens('(', ')')
assert not attachble_tokens('int', 'main')
assert not attachble_tokens('1', '2')
assert not attachble_tokens('return', '0')


# def group_tokens(tokens: list, start: list, end: list) -> list:
#     """Finds ``start`` tokens and concatenates tokens until ``end`` tokens is found, including the ``end`` tokens."""


def find_directives(content: str) -> list:
    directives = []
    for line in content.split('\n'):
        if line.startswith('#'):
            directives.append(line)
            content = content.replace(line, '')
            
    return directives

assert find_directives('#include <iostream>\n#define X 80\nint main() { return 0; }') == ['#include <iostream>', '#define X 80']


def write_minification(directives: list, content: str) -> str:
    """Writes the minified code."""
    minified = ''
    
    for d in directives:
        minified += d + '\n'
    
    minified += content
    
    with open('minified.cpp', 'w') as f:
        f.write(minified)


def minify(content: str):
    # Step 1. Remove any preprocessor directives and save them in a list for later use
    directives = find_directives(content)
    
    # Step 2. Remove newlines to make the content one line
    content = content.replace('\n', '')

    write_minification(directives, content)
    

if __name__ == '__main__':
    with open('main.cpp', 'r') as f:
        src = f.read()
        
        minify(src)