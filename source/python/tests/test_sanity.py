import smoldyn

print("Using smoldyn: %s, %s" % (smoldyn.__file__, smoldyn.__version__))

def test_module():
    print(smoldyn.__version__)

def test_model():
    model = smoldyn.Model()
    defines = model.define
    print(defines)
    defines['K_FWD'] = 0.001
    assert defines['K_FWD'] == 0.001
    defines['K_BACK'] = 1
    assert defines['K_BACK'] == 1
    defines['K_PROD'] = 1
    assert defines['K_PROD'] == 1
    defines['TEXTOUTPUT'] = True
    assert defines['TEXTOUTPUT'] == True
    print(defines)
    print(model.define)
    print(model)

if __name__ == "__main__":
    test_module()
    test_model()
