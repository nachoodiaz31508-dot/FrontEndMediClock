namespace FrontEndMediClock.Servicios;

// Guarda el dispositivo seleccionado para compartirlo entre paginas (T4/T5).
// Default id=1 (ESP_32_Prueba) segun alcance del MVP.
public class EstadoDispositivo
{
    public int DispositivoId { get; private set; } = 1;

    public event Action? Cambio;

    public void Seleccionar(int id)
    {
        if (DispositivoId == id)
        {
            return;
        }

        DispositivoId = id;
        Cambio?.Invoke();
    }
}
