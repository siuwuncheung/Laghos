NUM_PARALLEL_PROCESSORS=8
testNames=(offline online romhr qdeim restore)
case $subTestNum in
  1)
    $LAGHOS -p 4 -m data/square_gresho.mesh -rs 3 -ok 3 -ot 2 -tf 0.12 -s 7 -pa -offline -writesol -romsvds -no-romsns -romos
    ;;
  2)
    $LAGHOS -p 4 -m data/square_gresho.mesh -rs 3 -ok 3 -ot 2 -tf 0.12 -s 7 -pa -online -romgs -rdimx 4 -rdimv 20 -rdime 16 -soldiff -no-romsns -romos
    ;;
  3)
    $LAGHOS -p 4 -m data/square_gresho.mesh -rs 3 -ok 3 -ot 2 -tf 0.12 -s 7 -pa -online -romgs -rdimx 4 -rdimv 20 -rdime 16 -rdimfv 34 -rdimfe 39 -romhrprep -nsamx 18 -nsamv 3401 -nsame 128 -no-romsns -romos
    $LAGHOS_SERIAL -p 4 -m data/square_gresho.mesh -rs 3 -ok 3 -ot 2 -tf 0.12 -s 7 -pa -online -romgs -rdimx 4 -rdimv 20 -rdime 16 -rdimfv 34 -rdimfe 39 -romhr -nsamx 18 -nsamv 3401 -nsame 128 -no-romsns -romos
    ;;
  4)
    $LAGHOS -p 4 -m data/square_gresho.mesh -rs 3 -ok 3 -ot 2 -tf 0.12 -s 7 -pa -online -romgs -rdimx 4 -rdimv 20 -rdime 16 -rdimfv 34 -rdimfe 39 -romhrprep -sfacv 40 -sface 40 -qdeim -no-romsns -romos
    $LAGHOS_SERIAL -p 4 -m data/square_gresho.mesh -rs 3 -ok 3 -ot 2 -tf 0.12 -s 7 -pa -online -romgs -rdimx 4 -rdimv 20 -rdime 16 -rdimfv 34 -rdimfe 39 -romhr -sfacv 40 -sface 40 -qdeim -no-romsns -romos
    ;;
  5)
    $LAGHOS -p 4 -m data/square_gresho.mesh -rs 3 -ok 3 -ot 2 -tf 0.12 -s 7 -pa -restore -rdimx 4 -rdimv 20 -rdime 16 -soldiff -no-romsns -romos
    ;;
esac