NUM_PARALLEL_PROCESSORS=8
testNames=(offline online romhr restore)
case $subTestNum in
  1)
    $LAGHOS -p 3 -m data/box01_hex.mesh -rs 1 -tf 0.04 -cfl 0.05 -pa -offline -writesol -romsvds -no-romsns -romos
    ;;
  2)
    $LAGHOS -p 3 -m data/box01_hex.mesh -rs 1 -tf 0.04 -cfl 0.05 -pa -online -romgs -rdimx 1 -rdimv 4 -rdime 3 -soldiff -no-romsns -romos
    ;;
  3)
    $LAGHOS -p 3 -m data/box01_hex.mesh -rs 1 -tf 0.04 -cfl 0.05 -pa -online -romgs -rdimx 1 -rdimv 4 -rdime 3 -rdimfv 5 -rdimfe 4 -romhrprep -nsamx 6 -nsamv 448 -nsame 10 -no-romsns -romos
    $LAGHOS_SERIAL -p 3 -m data/box01_hex.mesh -rs 1 -tf 0.04 -cfl 0.05 -pa -online -romgs -rdimx 1 -rdimv 4 -rdime 3 -rdimfv 5 -rdimfe 4 -romhr -nsamx 6 -nsamv 448 -nsame 10 -no-romsns -romos
    ;;
  4)
    $LAGHOS -p 3 -m data/box01_hex.mesh -rs 1 -tf 0.04 -cfl 0.05 -pa -restore -rdimx 1 -rdimv 4 -rdime 3 -soldiff -no-romsns -romos
    ;;
esac