import smoldyn
print(dir(smoldyn))

print("Using smoldyn: %s, %s" % (smoldyn.__file__, smoldyn.__version__))

def test_module():
    print(smoldyn.__version__)
    print(dir(smoldyn))

if __name__ == "__main__":
    test_module()
