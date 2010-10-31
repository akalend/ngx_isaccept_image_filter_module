/*
 * Copyright (C) Alexandre Kalendarev 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct {
    ngx_int_t state;
} ngx_http_isaccept_ctx_t;

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;

static ngx_int_t ngx_http_isaccept_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_isaccept_filter_init(ngx_conf_t *cf);

static ngx_int_t ngx_http_isaccept_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_isaccept_handler_variable(ngx_http_request_t *r,
							ngx_http_variable_value_t *v, uintptr_t data);
	
static ngx_http_module_t  ngx_http_isaccept_filter_module_ctx = {
    ngx_http_isaccept_add_variables,      /* preconfiguration */
    ngx_http_isaccept_filter_init,        /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,								   /* create location configration */
    NULL								   /* merge location configration */
};

ngx_module_t  ngx_http_isaccept_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_isaccept_filter_module_ctx,         /* module context */
    NULL,								   /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_str_t  ngx_http_isaccept = ngx_string("isaccept");

static ngx_int_t
ngx_http_isaccept_header_filter(ngx_http_request_t *r)
{
	ngx_http_isaccept_ctx_t *ctx;
	
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_isaccept_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_http_set_ctx(r, ctx, ngx_http_isaccept_filter_module);
	ctx->state = 0;

	ngx_list_t headers = r->headers_in.headers;
	ngx_list_part_t *part;
	
	part = &headers.part;
	ngx_table_elt_t *header = part->elts;

	unsigned int i;
  for (i = 0 ;;i++) {
    if (i >= part->nelts) {
      if (part->next == NULL) {
        break;
      }
      part = part->next;
      header = part->elts;
      i = 0;
    }

    if (header[i].hash == 0) {
            continue;
	}

	u_char * p;
	if (header[i].key.len == 6 && !ngx_strcmp("Accept", header[i].key.data)) {
		p = header[i].value.data;
		unsigned int j;
		for (j = 0; j< header[i].value.len-6; j++) { 
			u_char *m = p+j;
			if (!ngx_strncmp(m,"image/*",7)) { 
					ctx->state = 1;
					break;
			}
		}
	}
  }	
    return ngx_http_next_header_filter(r);
}

static ngx_int_t
ngx_http_isaccept_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_isaccept_header_filter;
	
    return NGX_OK;
}

static ngx_int_t
ngx_http_isaccept_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &ngx_http_isaccept, NGX_HTTP_VAR_NOHASH | NGX_HTTP_VAR_NOCACHEABLE);
    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_isaccept_handler_variable;

    return NGX_OK;
}


static ngx_int_t
ngx_http_isaccept_handler_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
	ngx_http_isaccept_ctx_t *ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_isaccept_filter_module);
	
	if (ctx == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

	v->valid = 1;
    v->not_found = 0;

	u_char * buff = ngx_pcalloc(r->pool, 3);
    if (buff == NULL) {
        return NGX_ERROR;
    }

	v->len = 1;
	ngx_sprintf(buff, "%i", ctx->state); 
	v->data = buff;
	
	return NGX_OK;
}