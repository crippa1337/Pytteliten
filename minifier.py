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
assert not is_name('123')
assert not is_name('1a2b3c')
assert not is_name('')
assert not is_name(' ')
assert not is_name('\n')


def group_tokens(tokens: list, start: list, end: list) -> list:
    """Finds ``start`` tokens and concatenates tokens until ``end`` tokens is found, including the ``end`` tokens."""
    
    


if __name__ == '__main__':
    with open('main.cpp', 'r') as f:
        content = f.read()
        
        print(content)
        # TODO: Add includes to the top of the file
        
        # Remove newlines so that the content is one line
        content = content.replace('\n', '')
        print(content)
        