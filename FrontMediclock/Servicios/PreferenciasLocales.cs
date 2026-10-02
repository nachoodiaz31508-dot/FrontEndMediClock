using Microsoft.JSInterop;

namespace FrontEndMediClock.Servicios;

// Preferencias solo-front guardadas en localStorage del navegador.
// No llaman al backend: el nombre del paciente y el alias del
// dispositivo son datos de presentacion hasta que el servidor
// exponga perfiles reales.
public class PreferenciasLocales
{
    private const string ClavePaciente = "mediclock.nombrePaciente";

    private readonly IJSRuntime _js;

    public PreferenciasLocales(IJSRuntime js)
    {
        _js = js;
    }

    public Task<string?> ObtenerNombrePaciente() => Leer(ClavePaciente);

    public Task GuardarNombrePaciente(string nombre) => Guardar(ClavePaciente, nombre);

    public Task<string?> ObtenerAlias(int dispositivoId) => Leer($"mediclock.alias.{dispositivoId}");

    public Task GuardarAlias(int dispositivoId, string alias) => Guardar($"mediclock.alias.{dispositivoId}", alias);

    private async Task<string?> Leer(string clave)
    {
        try
        {
            return await _js.InvokeAsync<string?>("localStorage.getItem", clave);
        }
        catch
        {
            // Sin almacenamiento disponible: se usa el valor por defecto.
            return null;
        }
    }

    private async Task Guardar(string clave, string valor)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(valor))
            {
                await _js.InvokeVoidAsync("localStorage.removeItem", clave);
            }
            else
            {
                await _js.InvokeVoidAsync("localStorage.setItem", clave, valor);
            }
        }
        catch
        {
            // Sin almacenamiento disponible: la app sigue funcionando.
        }
    }
}
