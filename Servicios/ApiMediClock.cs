using System.Net;
using System.Net.Http.Json;
using FrontEndMediClock.Modelos;

namespace FrontEndMediClock.Servicios;

// Cliente del backend MediClock. Centraliza la URL base y el manejo de errores
// para que las paginas (T4/T5) solo consuman metodos simples en español.
public class ApiMediClock
{
    private readonly HttpClient _http;

    public ApiMediClock(HttpClient http)
    {
        _http = http;
    }

    // GET /api/dispositivos
    public async Task<List<Dispositivo>> ListarDispositivos()
    {
        var respuesta = await _http.GetAsync("api/dispositivos");
        await ValidarRespuesta(respuesta, "No se pudo obtener la lista de dispositivos.");
        return await respuesta.Content.ReadFromJsonAsync<List<Dispositivo>>() ?? new();
    }

    // GET /api/dispositivos/{id}/configuracion
    public async Task<ConfiguracionDispositivo> ObtenerConfiguracion(int id)
    {
        var respuesta = await _http.GetAsync($"api/dispositivos/{id}/configuracion");
        await ValidarRespuesta(respuesta, "No se pudo obtener la configuracion del dispositivo.");
        return await respuesta.Content.ReadFromJsonAsync<ConfiguracionDispositivo>()
            ?? throw new ExcepcionApi((int)respuesta.StatusCode, new() { "El servidor devolvio una configuracion vacia." });
    }

    // GET /api/dispositivos/{id}/alarmas
    public async Task<List<Alarma>> ListarAlarmas(int id)
    {
        var respuesta = await _http.GetAsync($"api/dispositivos/{id}/alarmas");
        await ValidarRespuesta(respuesta, "No se pudieron obtener las alarmas.");
        return await respuesta.Content.ReadFromJsonAsync<List<Alarma>>() ?? new();
    }

    // POST /api/dispositivos/{id}/alarmas con { diaSemana, numeroAlarma, hora }.
    // Devuelve la alarma creada. 409 si ya existe ese dia y numero.
    public async Task<Alarma> CrearAlarma(int id, Alarma datos)
    {
        var respuesta = await _http.PostAsJsonAsync(
            $"api/dispositivos/{id}/alarmas",
            new { datos.diaSemana, datos.numeroAlarma, datos.hora });
        await ValidarRespuesta(respuesta, "No se pudo crear la alarma.");
        return await respuesta.Content.ReadFromJsonAsync<Alarma>()
            ?? throw new ExcepcionApi((int)respuesta.StatusCode, new() { "El servidor devolvio una alarma vacia." });
    }

    // PUT /api/dispositivos/{id}/alarmas/{alarmaId} con { diaSemana, numeroAlarma, hora }.
    // Devuelve la alarma actualizada. 409 si choca con otra existente.
    public async Task<Alarma> ActualizarAlarma(int id, int alarmaId, Alarma datos)
    {
        var respuesta = await _http.PutAsJsonAsync(
            $"api/dispositivos/{id}/alarmas/{alarmaId}",
            new { datos.diaSemana, datos.numeroAlarma, datos.hora });
        await ValidarRespuesta(respuesta, "No se pudo actualizar la alarma.");
        return await respuesta.Content.ReadFromJsonAsync<Alarma>()
            ?? new Alarma
            {
                alarmaId = alarmaId,
                diaSemana = datos.diaSemana,
                numeroAlarma = datos.numeroAlarma,
                hora = datos.hora,
                dispositivoId = id,
            };
    }

    // DELETE /api/dispositivos/{id}/alarmas/{alarmaId} (204 sin contenido).
    // 409 si la alarma tiene eventos registrados y no se puede borrar.
    public async Task EliminarAlarma(int id, int alarmaId)
    {
        var respuesta = await _http.DeleteAsync($"api/dispositivos/{id}/alarmas/{alarmaId}");
        await ValidarRespuesta(respuesta, "No se pudo borrar la alarma.");
    }

    // GET /api/dispositivos/{id}/eventos?pagina=&tamanioPagina=
    // La paginacion la aplica el servidor (pagina desde 1).
    public async Task<List<Evento>> ListarEventos(int id, int pagina = 1, int tamanioPagina = 10)
    {
        var respuesta = await _http.GetAsync($"api/dispositivos/{id}/eventos?pagina={pagina}&tamanioPagina={tamanioPagina}");
        await ValidarRespuesta(respuesta, "No se pudieron obtener los eventos.");
        return await respuesta.Content.ReadFromJsonAsync<List<Evento>>() ?? new();
    }

    // Lanza ExcepcionApi si el estado no es exitoso.
    // Intenta leer el string[] del backend; si no hay cuerpo, usa un mensaje
    // en español neutro segun el codigo (404/409/400).
    private static async Task ValidarRespuesta(HttpResponseMessage respuesta, string mensajeGenerico)
    {
        if (respuesta.IsSuccessStatusCode)
        {
            return;
        }

        var mensajes = await LeerMensajes(respuesta);
        if (mensajes.Count == 0)
        {
            mensajes = new() { MensajePorDefecto(respuesta.StatusCode, mensajeGenerico) };
        }

        throw new ExcepcionApi((int)respuesta.StatusCode, mensajes);
    }

    private static async Task<List<string>> LeerMensajes(HttpResponseMessage respuesta)
    {
        try
        {
            var mensajes = await respuesta.Content.ReadFromJsonAsync<List<string>>();
            if (mensajes is { Count: > 0 })
            {
                return mensajes;
            }
        }
        catch
        {
            // No era un string[]: se intenta leer como detalle de problema.
        }

        // El backend devuelve 400 con formato detalle de problema
        // ({ errors: { Campo: [mensajes] } }); se extraen esos textos.
        try
        {
            using var documento = await respuesta.Content.ReadFromJsonAsync<System.Text.Json.JsonDocument>();
            var textos = new List<string>();
            if (documento is not null
                && documento.RootElement.TryGetProperty("errors", out var errores)
                && errores.ValueKind == System.Text.Json.JsonValueKind.Object)
            {
                foreach (var campo in errores.EnumerateObject())
                {
                    foreach (var texto in campo.Value.EnumerateArray())
                    {
                        if (texto.GetString() is string mensaje)
                        {
                            textos.Add(mensaje);
                        }
                    }
                }
            }

            return textos;
        }
        catch
        {
            // Cuerpo vacio o no JSON (ej: 204 o error de red): sin mensajes.
            return new();
        }
    }

    private static string MensajePorDefecto(HttpStatusCode codigo, string mensajeGenerico) => codigo switch
    {
        HttpStatusCode.NotFound => "No se encontro el recurso solicitado.",
        HttpStatusCode.Conflict => "La operacion entra en conflicto con un registro existente.",
        HttpStatusCode.BadRequest => "La solicitud no es valida. Revise los datos ingresados.",
        _ => mensajeGenerico,
    };
}
