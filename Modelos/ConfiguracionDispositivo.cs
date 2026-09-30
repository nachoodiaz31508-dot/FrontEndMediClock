namespace FrontEndMediClock.Modelos;

// Respuesta de GET /api/dispositivos/{id}/configuracion:
// estado actual del dispositivo con sus alarmas programadas.
public class ConfiguracionDispositivo
{
    public int dispositivoId { get; set; }

    public string nombre { get; set; } = string.Empty;

    public List<Alarma> alarmas { get; set; } = new();
}
