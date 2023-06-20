# Pytteliten
A chess engine designed to fit into 4kB.
Currently developed for the sole purpose of competing in the [TCEC 4K](https://wiki.chessdom.org/TCEC_4k_Rules).

<br>

## How does it work?
Pytteliten is currently just one ``.cpp`` file that is read by the minifier and remade into an identically behaving but vastly smaller source.
This minified source is appended to a launch script by the build script. When the launch script is run, it will compile an execute the minified source.
Together it's the launch script as well as the minified source that is counted towards the 4kB limit.

Pytteliten-mini does not support various parts of the UCI protocol that would otherwise be standard in a UCI engine and is only made to be compatible with
the tournament standards of TCEC 4K. The minifier is not a universal C++ minifier and is only made to support Pytteliten's source.

<br>

## Contributions
Development is mainly centered in the ``#pytteliten`` channel at the [Engine Programming Discord server](https://discord.com/invite/F6W6mMsTGN).

Contributions must be formatted with the ``.clang-format`` in the source and be compileable after minification. GitHub jobs handle these checks.
