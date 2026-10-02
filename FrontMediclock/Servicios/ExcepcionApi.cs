namespace FrontEndMediClock.Servicios;

// Error del backend: el API siempre devuelve string[] en fallos (400/404/409).
// Esta excepcion conserva el codigo HTTP y los mensajes para mostrarlos tal cual.
public class ExcepcionApi : Exception
{
    public int Codigo { get; }

    public List<string> Mensajes { get; }

    public ExcepcionApi(int codigo, List<string> mensajes)
        : base(string.Join(" ", mensajes))
    {
        Codigo = codigo;
        Mensajes = mensajes;
    }
}
