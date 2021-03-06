# esp32 forth computer

Computer made with ESP32 microcontroler

* Use https://github.com/fdivitto/fabgl (with a modification)
* Inspired by JupiterAce computer
* Inspired by ColorForth language

## Modification of FabGl libary

m_HVSync make public to calculate sync without make color (separate components)

in: src/dispdrivers/vgabasecontroller.h
```
  void setRawPixel(int x, int y, uint8_t rgb)    { VGA_PIXEL(x, y) = rgb; }

  // contains H and V signals for visible line	//********
  volatile uint8_t       m_HVSync;				//**** from above (protected)

protected:
```

##