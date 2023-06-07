import re

#################
# Name mangling #
#################

# KEY: TOKEN
# VALUE: MANGLED NAME
names = dict()
KEYWORDS = ['int', 'return', 'printf', 'struct', 'main', 'std', 'uint16_t', 'uint32_t', 'uint64_t', 'vector',
            'push_back', 'back', 'pop_back', 'reserve', 'cout', '__builtin_bswap64', '__builtin_ctzll', 'auto',
            'const', 'assert', 'endl', 'for', 'while', 'swap', 'bool', 'if', 'else', 'void', 'string', 'char', 'abs',
            'getline', 'split', 'break', 'length', 'switch', 'case', 'cin', 'istringstream', 'empty', 'continue', 'size',
            'default', 'using', 'namespace', 'int32_t', '__builtin_popcountll', 'stoi', 'chrono',
            'high_resolution_clock', 'duration_cast', 'milliseconds', 'now']

global counter, resets
counter = 65  # ASCII A
resets = 0  # Number of times the counter has exceeded reset back to A


def generate_name(token: str) -> str:
    """Generates a new name for a token. If the token has already been before, return that name to keep coherency."""
    global counter, resets

    # Already mangled
    if token in names:
        return names[token]

    # Generate a new name
    names[token] = chr(counter) * (resets + 1)

    # Jump to lowercase letters
    if counter == 90:  # previous char was ASCII Z
        counter = 96   # ASCII before 'a', because we increment after this to get to 'a'

    # Increment counter so that the next name is different
    counter += 1
    if counter > 122:  # ASCII z
        counter = 65
        resets += 1

    return names[token]


def fetch_tokens(content: str) -> list:
    """Fetches all the tokens from the source code. A token is a word, number, whitespace, symbol, etc."""
    return [t for t in re.split('([^\w])', content) if t]


assert fetch_tokens('int main() { return 0; }') == [
    'int', ' ', 'main', '(', ')', ' ', '{', ' ', 'return', ' ', '0', ';', ' ', '}']
assert fetch_tokens('hehe = 1;') == ['hehe', ' ', '=', ' ', '1', ';']
assert fetch_tokens('a_very_long_variable_name') == [
    'a_very_long_variable_name']
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


def is_keyword(token: str) -> bool:
    """Returns whether the token is a CPP keyword used in the engine."""
    return token in KEYWORDS


def renamable(token: str) -> bool:
    """Returns whether the token is renamable, i.e names that aren't types/keywords."""
    return not is_keyword(token) and is_name(token)


assert renamable('abcdefg')
assert renamable('a')
assert renamable('a_123')
assert not renamable('main')
assert not renamable('int')
assert not renamable('return')
assert not renamable('1000')


def attach_eligible(token: str) -> bool:
    """Returns whether a token is eligible to be attached together to save whitespace."""
    # Some c++ numbers have suffixes like 1ULL which is 1 as an unsigned long long.
    # We only check the first char of the token to be numeric to catch these cases.
    return token and not (is_name(token) or token[0].isnumeric())


assert attach_eligible(')')
assert attach_eligible('+')
assert attach_eligible('{')
assert not attach_eligible('a_name')
assert not attach_eligible('1')
assert not attach_eligible('main')


def attachable_tokens(first: str, second: str) -> bool:
    """Returns whether two tokens can be attached together to save whitespace."""
    return attach_eligible(first) or attach_eligible(second)


assert attachable_tokens('1', '+')
assert attachable_tokens('{', '}')
assert attachable_tokens('(', ')')
assert attachable_tokens('{', 'printf')
assert attachable_tokens('coolboy99', '+')
assert not attachable_tokens('int', 'main')
assert not attachable_tokens('1', '2')
assert not attachable_tokens('return', '0')


def group_tokens(token_list: list, start: list, end: list, include_end: bool = True) -> list:
    """Finds ``start`` tokens and concatenates everything from ``start``-``end``."""
    output = []

    # This is the start index where we look for first index such that [idx + len(start)] <=> start
    start_idx = 0

    while start_idx < len(token_list):

        # end must be after the start, so set it to start + 1
        end_idx = start_idx + 1

        # Verify that there is still space in the token list in the first condition.
        # In the second condition, check that start that was passed in ==
        # token_list starting at start_idx till start_idx + len(start).
        # Recollect that start_idx is just an idx, and start_idx + len(start) is a
        # subsequence of token_list of size len(start)
        if (start_idx + len(start) <= len(token_list)) and (start == token_list[start_idx:start_idx + len(start)]):

            # Search all possible end idxs from the end of the start to the end of the string.
            # Give padding for the size of the end token.
            for possible_end_idx in range(end_idx + len(start) - 1, len(token_list) - len(end) + 1):

                # Similar to the first condition, check if from end idx to len(end) == passed in end.
                if end == token_list[possible_end_idx: possible_end_idx + len(end)]:
                    # Just include the whole end if that param is true.
                    end_idx = possible_end_idx + \
                        len(end) if include_end else possible_end_idx
                    break

        # Self-explanatory, join everything from start to end. If we never found anything this is
        # start:start+1 because we set end_idx to start + 1 at the beginning.
        # This adds just the token if we found nothing.
        output.append("".join(token_list[start_idx:end_idx]))

        # We searched everything from start to end, so no need to research. set start to old end.
        start_idx = end_idx

    return output


assert group_tokens(["a", "b", "c", "d"], ["a"], ["b"]) == ["ab", "c", "d"]
assert group_tokens(["a", "b", "c", "d"], ["b"], ["c"]) == ["a", "bc", "d"]
assert group_tokens(["a", "b", "c", "d"], ["c"], ["d"]) == ["a", "b", "cd"]
assert group_tokens(["a", "b", "c", "d"], ["a"], ["d"]) == ["abcd"]
assert group_tokens(["a", "b", "c", "d"], ["c"], [
    "b"]) == ["a", "b", "c", "d"]
assert group_tokens(["a", "b", "c", "d"], ["b"], [
    "e"]) == ["a", "b", "c", "d"]
assert group_tokens(["a", "b", "c", "d"], ["b"], [
    "b"]) == ["a", "b", "c", "d"]
assert group_tokens(["a", "b", "c", "d"], ["a", "b"], ["c"]) == ["abc", "d"]
assert group_tokens(["a", "b", "c", "d"], ["a"], ["b", "c"]) == ["abc", "d"]
assert group_tokens(["a", "b", "c", "d"], ["a", "b", "c"], ["d"]) == ["abcd"]
assert group_tokens(["a", "b", "c", "d"], ["a"], ["c"], True) == ["abc", "d"]
assert group_tokens(["a", "b", "c", "d"], ["a"], [
    "c"], False) == ["ab", "c", "d"]
assert group_tokens(["a", "b", "c", "d"], ["c"], [
    "e"]) == ["a", "b", "c", "d"]
assert group_tokens(["a", "b", "c", "d"], ["e"], [
    "c"]) == ["a", "b", "c", "d"]


def minify(content: str):
    tokens = fetch_tokens(content)

    # Create token groups such as comments, strings, etc.
    tokens = group_tokens(tokens, ['"'], ['"'])              # Strings
    tokens = group_tokens(tokens, ['/', '*'], ['*', '/'])    # Block comments
    tokens = group_tokens(tokens, ['[', '['], [']', ']'])    # Attributes
    tokens = group_tokens(tokens, ['#', 'include'], ['\n'])  # Includes

    # Deletion regions
    tokens = group_tokens(tokens, ['/', '/', ' ', 'minify', ' ', 'enable', ' ', 'filter', ' ', 'delete'],
                          ['/', '/', ' ', 'minify', ' ', 'disable', ' ', 'filter', ' ', 'delete'])

    tokens = group_tokens(tokens, ['/', '/'], ['\n'], False)  # Line comments
    tokens = group_tokens(tokens, ["'"], ["'"])               # Chars

    new_tokens = []
    prev = None

    # Replace true and false with 1 and 0
    names['true'] = '1'
    names['false'] = '0'

    for kw in KEYWORDS:
        names[kw] = kw

    for token in tokens:
        # # Exclude the token if it is:
        # Line comments and deletion regions
        if token.startswith('//'):
            continue
        # Block comment
        elif token.startswith('/*'):
            continue
        # Attribute
        elif token.startswith('[['):
            continue

        # Exclude newlines, spaces, consts...
        if token == '\n' or token.isspace() or token == 'const':
            continue

        # Add a seperator between tokens that can't be attached to each other.
        # For example: Two names (int main)
        if prev and not attachable_tokens(prev, token):
            new_tokens.append(' ')

        # If the token is a name, but not a keyword, we mangle it.
        if renamable(token):
            token = generate_name(token)

        prev = token
        new_tokens.append(token)

    with open('pytteliten-mini.cpp', 'w') as f:
        f.write(''.join(new_tokens))


if __name__ == '__main__':
    with open('main.cpp', 'r') as f:
        src = f.read()

        minify(src)
