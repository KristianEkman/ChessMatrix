../cutechess/cutechess/build/cutechess-cli \
 -engine cmd=../texel/build/texel proto=uci name=Texel2000 option.UCI_LimitStrength=true option.UCI_Elo=2000 \
 -engine cmd=./chessmatrix proto=uci name=NEW \
 -each tc=30+0.01 \
 -games 400 \
 -repeat \
 -concurrency 4 \
 -openings file=UHO_4060_v4.epd format=epd order=random