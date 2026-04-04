../cutechess/cutechess/build/cutechess-cli \
 -engine cmd=./chessmatrix_master proto=uci name=OLD \
 -engine cmd=./chessmatrix proto=uci name=NEW \
 -each tc=5+0.01 \
 -games 50 \
 -repeat \
 -concurrency 4 \
 -openings file=UHO_4060_v4.epd format=epd order=random