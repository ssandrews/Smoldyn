import smoldyn
print("Using smoldyn: %s, %s" % (smoldyn.__file__, smoldyn.__version__))

def test_module():
    print(smoldyn.__version__)
    print(dir(smoldyn))
    smoldyn.simulate('../examples/S10_boxes/box.txt', "")

if __name__ == "__main__":
    test_module()
